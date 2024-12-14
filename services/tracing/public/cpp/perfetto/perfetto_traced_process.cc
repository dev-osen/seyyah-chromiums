// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/public/cpp/perfetto/perfetto_traced_process.h"

#include "base/command_line.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "base/trace_event/trace_config.h"
#include "base/tracing/perfetto_platform.h"
#include "build/build_config.h"
#include "services/tracing/public/cpp/perfetto/custom_event_recorder.h"
#include "services/tracing/public/cpp/perfetto/metadata_data_source.h"
#include "services/tracing/public/cpp/perfetto/perfetto_tracing_backend.h"
#include "services/tracing/public/cpp/perfetto/track_name_recorder.h"
#include "services/tracing/public/cpp/stack_sampling/tracing_sampler_profiler.h"
#include "services/tracing/public/cpp/system_metrics_sampler.h"
#include "services/tracing/public/cpp/trace_startup.h"
#include "services/tracing/public/cpp/traced_process_impl.h"
#include "services/tracing/public/cpp/tracing_features.h"
#include "services/tracing/public/mojom/tracing_service.mojom.h"
#include "third_party/perfetto/include/perfetto/tracing/tracing.h"

#if BUILDFLAG(IS_WIN)
#include "components/tracing/common/etw_system_data_source_win.h"
#endif

#if BUILDFLAG(IS_POSIX) || BUILDFLAG(IS_FUCHSIA)
// As per 'gn help check':
/*
  If you have conditional includes, make sure the build conditions and the
  preprocessor conditions match, and annotate the line with "nogncheck" (see
  "gn help nogncheck" for an example).
*/
// We add the nogncheck to ensure this doesn't trigger incorrect errors on
// non-android builds.
#include "third_party/perfetto/include/perfetto/tracing/default_socket.h"  // nogncheck
#endif  // BUILDFLAG(IS_POSIX)

namespace tracing {
namespace {

void OnPerfettoLogMessage(perfetto::base::LogMessageCallbackArgs args) {
  // Perfetto levels start at 0, base's at -1.
  int severity = static_cast<int>(args.level) - 1;
  ::logging::LogMessage(args.filename, args.line, severity).stream()
      << args.message;
}

#if BUILDFLAG(IS_POSIX) && !BUILDFLAG(IS_ANDROID)
// The async socket connection function passed to the client library for
// connecting the producer socket in the browser process via mojo IPC.
// |cb| is a callback from within the client library this function calls when
// the socket is opened.
void ConnectProducerSocketViaMojo(perfetto::CreateSocketCallback cb,
                                  uint32_t retry_delay_ms) {
  // Binary backoff with a max of 30 sec.
  constexpr uint32_t kMaxRetryMs = 30 * 1000;
  auto next_retry_delay_ms = std::min(retry_delay_ms * 2, kMaxRetryMs);
  // Delayed reconnect function is bound with the increased retry delay.
  auto delayed_reconnect_fn = [cb, next_retry_delay_ms]() {
    ConnectProducerSocketViaMojo(cb, next_retry_delay_ms);
  };

  auto& remote = TracedProcessImpl::GetInstance()->system_tracing_service();
  if (!remote.is_bound()) {
    // Retry if the mojo remote is not bound yet.
    return PerfettoTracedProcess::GetTaskRunner()->PostDelayedTask(
        delayed_reconnect_fn, retry_delay_ms);
  }

  auto callback = base::BindOnce(
      [](perfetto::CreateSocketCallback cb,
         std::function<void()> delayed_reconnect_fn, uint32_t retry_delay_ms,
         base::File file) {
        if (!file.IsValid()) {
          return PerfettoTracedProcess::GetTaskRunner()->PostDelayedTask(
              delayed_reconnect_fn, retry_delay_ms);
        }

        // Success, call |cb| into the Perfetto client library with a valid
        // socket handle.
        cb(file.TakePlatformFile());
      },
      cb, delayed_reconnect_fn, retry_delay_ms);

  // Open the socket remotely using Mojo.
  remote->OpenProducerSocket(std::move(callback));
}

// Wrapper for |ConnectProducerSocketViaMojo| to be used as a function pointer.
void ConnectProducerSocketAsync(perfetto::CreateSocketCallback cb) {
  ConnectProducerSocketViaMojo(std::move(cb), 100);
}
#endif

}  // namespace

PerfettoTracedProcess::DataSourceBase::DataSourceBase(const std::string& name)
    : name_(name) {
  DCHECK(!name.empty());
  DETACH_FROM_SEQUENCE(perfetto_sequence_checker_);
}

PerfettoTracedProcess::DataSourceBase::~DataSourceBase() = default;

void PerfettoTracedProcess::DataSourceBase::StartTracing(
    uint64_t data_source_id,
    PerfettoProducer* producer,
    const perfetto::DataSourceConfig& data_source_config) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(perfetto_sequence_checker_);

  data_source_id_ = data_source_id;
  // Producer may already be set if startup tracing in TraceEventDataSource.
  DCHECK(!producer_ || producer_ == producer) << name_;
  producer_ = producer;
  StartTracingImpl(producer_, data_source_config);
}

void PerfettoTracedProcess::DataSourceBase::StopTracing(
    base::OnceClosure stop_complete_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(perfetto_sequence_checker_);

  StopTracingImpl(base::BindOnce(
      [](DataSourceBase* self, base::OnceClosure original_callback) {
        DCHECK_CALLED_ON_VALID_SEQUENCE(self->perfetto_sequence_checker_);
        self->producer_ = nullptr;
        if (original_callback)
          std::move(original_callback).Run();
      },
      this,  // OK to capture |this| because the callback is called by |this|.
      std::move(stop_complete_callback)));
}

void PerfettoTracedProcess::DataSourceBase::StartTracingImpl(
    PerfettoProducer* producer,
    const perfetto::DataSourceConfig& data_source_config) {}

void PerfettoTracedProcess::DataSourceBase::StopTracingImpl(
    base::OnceClosure stop_complete_callback) {
  if (stop_complete_callback)
    std::move(stop_complete_callback).Run();
}

void PerfettoTracedProcess::DataSourceBase::Flush(
    base::RepeatingClosure flush_complete_callback) {
  base::TrackEvent::Flush();
  if (flush_complete_callback)
    std::move(flush_complete_callback).Run();
}

base::SequencedTaskRunner*
PerfettoTracedProcess::DataSourceBase::GetTaskRunner() {
  return PerfettoTracedProcess::Get()
      ->GetTaskRunner()
      ->GetOrCreateTaskRunner()
      .get();
}

// static
PerfettoTracedProcess* PerfettoTracedProcess::Get() {
  static base::NoDestructor<PerfettoTracedProcess> traced_process;
  return traced_process.get();
}

PerfettoTracedProcess::PerfettoTracedProcess()
    : platform_(
          std::make_unique<base::tracing::PerfettoPlatform>(GetTaskRunner())),
      tracing_backend_(std::make_unique<PerfettoTracingBackend>()) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

PerfettoTracedProcess::~PerfettoTracedProcess() {}

void PerfettoTracedProcess::SetConsumerConnectionFactory(
    ConsumerConnectionFactory factory,
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  tracing_backend_->SetConsumerConnectionFactory(factory, task_runner);
}

void PerfettoTracedProcess::ConnectProducer(
    mojo::PendingRemote<mojom::PerfettoService> perfetto_service) {
  tracing_backend_->OnProducerConnected(std::move(perfetto_service));
}

// We never destroy the taskrunner as we may need it for cleanup
// of TraceWriters in TLS, which could happen after the PerfettoTracedProcess
// is deleted.
// static
base::tracing::PerfettoTaskRunner* PerfettoTracedProcess::GetTaskRunner() {
  static base::NoDestructor<base::tracing::PerfettoTaskRunner> task_runner(
      nullptr);
  return task_runner.get();
}

// static
std::unique_ptr<PerfettoTracedProcess::TestHandle>
PerfettoTracedProcess::SetupForTesting(
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  // Make sure Perfetto was properly torn down in any prior tests.
  DCHECK(!perfetto::Tracing::IsInitialized());
  GetTaskRunner()->ResetTaskRunnerForTesting(task_runner);
  PerfettoTracedProcess::Get()->tracing_backend_ =
      std::make_unique<PerfettoTracingBackend>();
  PerfettoTracedProcess::Get()->OnThreadPoolAvailable(
      /* enable_consumer */ true);
  // Disassociate the PerfettoTracedProcess from any prior task runner.
  DETACH_FROM_SEQUENCE(PerfettoTracedProcess::Get()->sequence_checker_);
  return std::make_unique<TestHandle>();
}

void PerfettoTracedProcess::RequestStartupTracing(
    const perfetto::TraceConfig& config,
    const perfetto::Tracing::SetupStartupTracingOpts& opts) {
  if (thread_pool_started_) {
    perfetto::Tracing::SetupStartupTracingBlocking(config, opts);
  } else {
    saved_config_ = config;
    saved_opts_ = opts;
    startup_tracing_needed_ = true;
  }
}

void PerfettoTracedProcess::SetupClientLibrary(bool enable_consumer) {
  perfetto::TracingInitArgs init_args;
  init_args.platform = platform_.get();
  init_args.custom_backend = tracing_backend_.get();
  init_args.backends |= perfetto::kCustomBackend;
  init_args.shmem_batch_commits_duration_ms = 1000;
  init_args.shmem_size_hint_kb = 4 * 1024;
  init_args.shmem_direct_patching_enabled = true;
  init_args.use_monotonic_clock = true;
  init_args.disallow_merging_with_system_tracks = true;
#if BUILDFLAG(IS_POSIX)
  if (ShouldSetupSystemTracing()) {
    init_args.backends |= perfetto::kSystemBackend;
    init_args.tracing_policy = this;
#if BUILDFLAG(IS_POSIX) && !BUILDFLAG(IS_ANDROID)
    auto type =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII("type");
    if (!type.empty()) {  // Sandboxed. Need to delegate to the browser process
                          // using Mojo.
      init_args.create_socket_async = ConnectProducerSocketAsync;
    }
#endif
  }
#endif
  // Proxy perfetto log messages into Chrome logs, so they are retained on all
  // platforms. In particular, on Windows, Perfetto's stderr log messages are
  // not reliable.
  init_args.log_message_callback = &OnPerfettoLogMessage;
  perfetto::Tracing::Initialize(init_args);

  base::TrackEvent::Register();
  tracing::TracingSamplerProfiler::RegisterDataSource();
  // SystemMetricsSampler will be started when enabling
  // kSystemMetricsSourceName.
  tracing::SystemMetricsSampler::Register(/*system_wide=*/enable_consumer);
  if (enable_consumer) {
    // Metadata only needs to be installed in the browser process.
    tracing::MetadataDataSource::Register();
#if BUILDFLAG(IS_WIN)
    // Etw Data Source only needs to be installed in the browser process.
    tracing::EtwSystemDataSource::Register();
#endif
  }
  TrackNameRecorder::GetInstance();
  CustomEventRecorder::GetInstance();
}

void PerfettoTracedProcess::OnThreadPoolAvailable(bool enable_consumer) {
  thread_pool_started_ = true;
  SetupClientLibrary(enable_consumer);

  if (startup_tracing_needed_) {
    perfetto::Tracing::SetupStartupTracingBlocking(saved_config_, saved_opts_);
    startup_tracing_needed_ = false;
  }
}

void PerfettoTracedProcess::SetAllowSystemTracingConsumerCallback(
    base::RepeatingCallback<bool()> callback) {
  base::AutoLock lock(allow_system_consumer_lock_);
  DCHECK(!allow_system_consumer_callback_ || !callback);
  allow_system_consumer_callback_ = std::move(callback);
  allow_system_consumer_callback_runner_ =
      base::SequencedTaskRunner::GetCurrentDefault();
}

void PerfettoTracedProcess::SetAllowSystemTracingConsumerForTesting(
    bool enabled) {
  base::AutoLock lock(allow_system_consumer_lock_);
  system_consumer_enabled_for_testing_ = enabled;
}

void PerfettoTracedProcess::ShouldAllowConsumerSession(
    const perfetto::TracingPolicy::ShouldAllowConsumerSessionArgs& args) {
  // Consumer connections should only be attempted in the browser process.
  CHECK(base::CommandLine::ForCurrentProcess()
            ->GetSwitchValueASCII("type")
            .empty());

  // Integrated tracing backends are always allowed.
  if (args.backend_type != perfetto::BackendType::kSystemBackend) {
    args.result_callback(true);
    return;
  }

  // System backend is only allowed in tests or if the embedder provided a
  // callback that allows it.
  ShouldAllowSystemConsumerSession(args.result_callback);
}

void PerfettoTracedProcess::ShouldAllowSystemConsumerSession(
    std::function<void(bool)> result_callback) {
  base::AutoLock lock(allow_system_consumer_lock_);

  if (system_consumer_enabled_for_testing_) {
    result_callback(true);
    return;
  }
  if (!allow_system_consumer_callback_) {
    result_callback(false);
    return;
  }

  if (!allow_system_consumer_callback_runner_->RunsTasksInCurrentSequence()) {
    allow_system_consumer_callback_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&PerfettoTracedProcess::ShouldAllowSystemConsumerSession,
                       base::Unretained(this), result_callback));
    return;
  }

  bool result = allow_system_consumer_callback_.Run();
  result_callback(result);
}

PerfettoTracedProcess::TestHandle::~TestHandle() {
  // TODO(skyostil): We only uninitialize Perfetto for now, but there may also
  // be other tracing-related state which should not leak between tests.
  perfetto::Tracing::ResetForTesting();
  PerfettoTracedProcess::Get()->tracing_backend_ = nullptr;
}

}  // namespace tracing
