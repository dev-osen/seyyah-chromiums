#ifndef SEYYAH_RENDER_OBSERVER_H_
#define SEYYAH_RENDER_OBSERVER_H_

#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame.h"


class SeyyahRenderObserver : public content::RenderFrameObserver {
public:
  explicit SeyyahRenderObserver(content::RenderFrame* render_frame);
  ~SeyyahRenderObserver() override;

  // V8 bağlamı oluşturulduğunda tetiklenir
  void DidCreateScriptContext(v8::Local<v8::Context> context, int world_id) override;

  void OnDestruct() override;

private:
  content::RenderFrame* render_frame_;

};

#endif // SEYYAH_RENDER_OBSERVER_H_
