#include "seyyah_v8_manipulator.h"
#include "seyyah_system.h"
#include "base/logging.h"
#include <iostream>

#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/network_connection_tracker.h"
#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "url/gurl.h"




void Run(v8::Isolate* isolate, v8::Local<v8::Context> context, const std::string& page_key){

  auto callback = [](std::shared_ptr<SeyyahV8Manipulator> protector) {
    if(protector) {
      protector->Protect();
      LOG(WARNING) << "[SEYYAH][SV8M]: Success.";
    } else {
      LOG(ERROR) << "[SEYYAH][SV8M]: ERROR.";
    }
  };

  SeyyahV8Manipulator::CreateFromServer(isolate, context, page_key, callback);
}


void SeyyahV8Manipulator::CreateFromServer(v8::Isolate* isolate,
                                           v8::Local<v8::Context> context,
                                           const std::string& page_key,
                                           std::function<void(std::shared_ptr<SeyyahV8Manipulator>)> callback) {
  nlohmann::json jsonData;
  {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = cache_.find(page_key);
    if(it != cache_.end()) {
      jsonData = it->second;
    }
  }

  if(!jsonData.is_null()) {
    auto protector = std::make_shared<SeyyahV8Manipulator>(isolate, context, jsonData, page_key);
    callback(protector);
    return;
  }

  GURL url(SeyyahSystem::GetSeyyahServer() + "/fp/" + page_key);
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->method = "GET";

  auto loader = network::SimpleURLLoader::Create(std::move(request), TRAFFIC_ANNOTATION_FOR_TESTS);
  loader->SetAllowHttpErrorResults(true);

  // Şimdi, shared_ptr kullanarak oluşturuyoruz
  auto protector = std::make_shared<SeyyahV8Manipulator>(isolate, context, nlohmann::json(), page_key);

  // Download işlemini başlatıyoruz
  loader->DownloadToString(
        network::NetworkConnectionTracker::GetInstance()->GetURLLoaderFactory(),
        base::BindOnce(&SeyyahV8Manipulator::OnURLLoadComplete, protector, std::move(loader), callback),
        1024 * 1024  // Maksimum yanıt boyutu (1MB)
    );

  protector->url_loader_ = std::move(loader);
}



void SeyyahV8Manipulator::OnURLLoadComplete(std::shared_ptr<SeyyahV8Manipulator> protector,
                                            std::shared_ptr<network::SimpleURLLoader> loader,
                                            std::function<void(std::shared_ptr<SeyyahV8Manipulator>)> callback,
                                            std::unique_ptr<std::string> response_body) {

  if (!response_body) {
    LOG(ERROR) << "[SEYYAH][SV8M] Error-1: Sunucudan veri alınamadı.";
    callback(nullptr);
    return;
  }

  nlohmann::json jsonData;
  try {
    jsonData = nlohmann::json::parse(*response_body);
  } catch(const nlohmann::json::parse_error& e) {
    LOG(ERROR) << "[SEYYAH][SV8M] Error-1: JSON parse hatası: " << e.what();
    callback(nullptr);
    return;
  }

  {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_[protector->GetPageKey()] = jsonData;
  }

  protector->fingerprintData_ = jsonData;

  callback(protector);
}







// Manipulator kodlari buradan sonra basliyor.

std::unordered_map<std::string, nlohmann::json> SeyyahV8Manipulator::cache_;
std::mutex SeyyahV8Manipulator::cache_mutex_;

SeyyahV8Manipulator::SeyyahV8Manipulator(v8::Isolate* isolate, v8::Local<v8::Context> context, const nlohmann::json& jsonFingerprint, const std::string& page_key)
    : isolate_(isolate), context_(isolate, context), fingerprintData_(jsonFingerprint), page_key_(page_key)  {}

SeyyahV8Manipulator::~SeyyahV8Manipulator() {
  context_.Reset();
}

v8::Local<v8::String> SeyyahV8Manipulator::GetV8String(const std::string& value) {
  return v8::String::NewFromUtf8(isolate_, value.c_str()).ToLocalChecked();
}

v8::Local<v8::Integer> SeyyahV8Manipulator::GetV8Integer(int value) {
  return v8::Integer::New(isolate_, value);
}

v8::Local<v8::Number> SeyyahV8Manipulator::GetV8Number(double value) {
  return v8::Number::New(isolate_, value);
}

v8::Local<v8::Boolean> SeyyahV8Manipulator::GetV8Boolean(bool value) {
  return v8::Boolean::New(isolate_, value);
}

v8::Local<v8::Value> SeyyahV8Manipulator::GetV8Value(const nlohmann::json& value) {
  if (value.is_string()) {
    return GetV8String(value.get<std::string>());
  } else if (value.is_number_integer()) {
    return GetV8Integer(value.get<int>());
  } else if (value.is_number_float()) {
    return GetV8Number(value.get<double>());
  } else if (value.is_boolean()) {
    return GetV8Boolean(value.get<bool>());
  } else {
    return v8::Undefined(isolate_);
  }
}


void SeyyahV8Manipulator::Protect() {
    SetPageKey();
    OverrideNavigator();
    OverrideScreen();
    OverrideCanvas();
    OverrideWebGL();
    OverrideAudioContext();
    OverrideFonts();
    OverrideBatteryStatus();
    OverrideNetworkInformation();
    OverrideWebRTC();
    OverrideDeviceOrientation();
    OverrideMediaDevices();
    OverrideStorage();
    OverridePerformance();
    OverridePermissions();
    OverrideReferrer();
    OverrideExtensions();
    OverrideTouchSupport();
    OverrideMediaCapabilities();
    OverrideStorageEstimates();
    OverrideSVGRendering();
    OverridePageInteractions();
    OverrideComputedStyles();
    OverrideColorScheme();
}


void SeyyahV8Manipulator::SetPageKey(){
  v8::HandleScope handle_scope(isolate_);
  v8::Local<v8::Context> context = context_.Get(isolate_);
  v8::Context::Scope context_scope(context);


  v8::Local<v8::String> key_name = v8::String::NewFromUtf8(isolate_, "seyyah_page_key", v8::NewStringType::kNormal).ToLocalChecked();
  v8::Local<v8::String> key_value = v8::String::NewFromUtf8(isolate_, page_key_.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
  bool set_success = context->Global()->Set(context, key_name, key_value).FromMaybe(false);

}


void SeyyahV8Manipulator::OverrideNavigator() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("navigator")) return;

    v8::Local<v8::Object> navigator = context->Global()->Get(context, GetV8String("navigator")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["navigator"].items()) {
        navigator->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }
}

void SeyyahV8Manipulator::OverrideScreen() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("screen")) return;

    v8::Local<v8::Object> screen = context->Global()->Get(context, GetV8String("screen")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["screen"].items()) {
        screen->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }
}

void SeyyahV8Manipulator::OverrideCanvas() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("canvas")) return;

    v8::Local<v8::Object> canvas = context->Global()->Get(context, GetV8String("HTMLCanvasElement")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["canvas"].items()) {
        canvas->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel Canvas işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideWebGL() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("webgl")) return;

    v8::Local<v8::Object> webgl = context->Global()->Get(context, GetV8String("WebGLRenderingContext")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["webgl"].items()) {
        webgl->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel WebGL işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideAudioContext() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("audioContext")) return;

    v8::Local<v8::Object> audioContext = context->Global()->Get(context, GetV8String("AudioContext")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["audioContext"].items()) {
        audioContext->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel AudioContext işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideFonts() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("fonts")) return;

    v8::Local<v8::Object> fonts = context->Global()->Get(context, GetV8String("Font")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["fonts"].items()) {
        fonts->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel Fonts işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideBatteryStatus() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("battery")) return;

    v8::Local<v8::Object> battery = context->Global()->Get(context, GetV8String("BatteryManager")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["battery"].items()) {
        battery->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel BatteryStatus işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideNetworkInformation() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("network")) return;

    v8::Local<v8::Object> network = context->Global()->Get(context, GetV8String("navigator")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["network"].items()) {
        network->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel NetworkInformation işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideWebRTC() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("webrtc")) return;

    v8::Local<v8::Object> webrtc = context->Global()->Get(context, GetV8String("RTCPeerConnection")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["webrtc"].items()) {
        webrtc->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel WebRTC işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideDeviceOrientation() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("deviceOrientation")) return;

    v8::Local<v8::Object> deviceOrientation = context->Global()->Get(context, GetV8String("DeviceOrientationEvent")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["deviceOrientation"].items()) {
        deviceOrientation->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel DeviceOrientation işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideMediaDevices() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("mediaDevices")) return;

    v8::Local<v8::Object> mediaDevices = context->Global()->Get(context, GetV8String("navigator")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["mediaDevices"].items()) {
        mediaDevices->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel MediaDevices işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideStorage() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("storage")) return;

    v8::Local<v8::Object> storage = context->Global()->Get(context, GetV8String("Storage")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["storage"].items()) {
        storage->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel Storage işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverridePerformance() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("performance")) return;

    v8::Local<v8::Object> performance = context->Global()->Get(context, GetV8String("performance")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["performance"].items()) {
        performance->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel Performance işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverridePermissions() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("permissions")) return;

    v8::Local<v8::Object> permissions = context->Global()->Get(context, GetV8String("navigator")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["permissions"].items()) {
        permissions->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel Permissions işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideReferrer() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("referrer")) return;

    v8::Local<v8::Object> document = context->Global()->Get(context, GetV8String("document")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["referrer"].items()) {
        document->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel Referrer işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideExtensions() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("extensions")) return;

    v8::Local<v8::Object> extensions = context->Global()->Get(context, GetV8String("chrome")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["extensions"].items()) {
        extensions->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel Extensions işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideTouchSupport() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("touchSupport")) return;

    v8::Local<v8::Object> navigator = context->Global()->Get(context, GetV8String("navigator")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["touchSupport"].items()) {
        navigator->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel TouchSupport işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideMediaCapabilities() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("mediaCapabilities")) return;

    v8::Local<v8::Object> mediaCapabilities = context->Global()->Get(context, GetV8String("MediaCapabilities")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["mediaCapabilities"].items()) {
        mediaCapabilities->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel MediaCapabilities işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideStorageEstimates() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("storageEstimates")) return;

    v8::Local<v8::Object> storageEstimate = context->Global()->Get(context, GetV8String("storageEstimate")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["storageEstimates"].items()) {
        storageEstimate->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel StorageEstimates işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideSVGRendering() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("svgRendering")) return;

    v8::Local<v8::Object> svgRendering = context->Global()->Get(context, GetV8String("SVGElement")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["svgRendering"].items()) {
        svgRendering->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel SVGRendering işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverridePageInteractions() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("pageInteractions")) return;

    v8::Local<v8::Object> pageInteractions = context->Global()->Get(context, GetV8String("PageInteractions")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["pageInteractions"].items()) {
        pageInteractions->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel PageInteractions işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideComputedStyles() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("computedStyles")) return;

    v8::Local<v8::Object> computedStyles = context->Global()->Get(context, GetV8String("getComputedStyle")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["computedStyles"].items()) {
        computedStyles->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel ComputedStyles işlemleri burada eklenebilir
}

void SeyyahV8Manipulator::OverrideColorScheme() {
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = context_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    if (!fingerprintData_.contains("colorScheme")) return;

    v8::Local<v8::Object> colorScheme = context->Global()->Get(context, GetV8String("matchMedia")).ToLocalChecked().As<v8::Object>();

    for (auto& [key, value] : fingerprintData_["colorScheme"].items()) {
        colorScheme->Set(context, GetV8String(key), GetV8Value(value)).Check();
    }

    // Özel ColorScheme işlemleri burada eklenebilir
}