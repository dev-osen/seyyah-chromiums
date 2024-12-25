#include "seyyah_render_observer.h"
#include "seyyah_render_observer_manager.h"
#include "seyyah_page_key_manager.h"
#include "seyyah_v8_manipulator.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8.h"
#include "base/logging.h"

SeyyahRenderObserver::SeyyahRenderObserver(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame), render_frame_(render_frame) {

  SeyyahRenderObserverManager::Instance().AddObserver(this);
}

SeyyahRenderObserver::~SeyyahRenderObserver() {
  SeyyahRenderObserverManager::Instance().RemoveObserver(this);
}

void SeyyahRenderObserver::DidCreateScriptContext(v8::Local<v8::Context> context, int world_id) {

  LOG(WARNING) << "[SEYYAH][RENDER]: START";

  int routing_id = render_frame_->GetRoutingID();
  std::string page_unique_key = SeyyahPageKeyManager::Instance().GetRoutingKey(routing_id);

  LOG(WARNING) << "[SEYYAH][RENDER] STEP-1: SPK: " << page_unique_key << " / routing_id: " << routing_id;

  if(page_unique_key == "")
    return;

  LOG(WARNING) << "[SEYYAH][RENDER] STEP-2";

  v8::Isolate* isolate = context->GetIsolate();
  if(!isolate)
    return;

  LOG(WARNING) << "[SEYYAH][RENDER] STEP-3";

  SeyyahV8Manipulator::Run(isolate, context, page_unique_key);

  LOG(WARNING) << "[SEYYAH][RENDER]: COMPLETED";
}

void SeyyahRenderObserver::OnDestruct() {
  delete this;
}
