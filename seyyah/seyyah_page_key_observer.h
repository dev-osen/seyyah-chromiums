#ifndef SEYYAH_PAGE_KEY_OBSERVER_H_
#define SEYYAH_PAGE_KEY_OBSERVER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_frame_host.h"
#include <string>

class SeyyahPageKeyObserver : public content::WebContentsObserver {

public:
  explicit SeyyahPageKeyObserver(content::WebContents* web_contents);
  ~SeyyahPageKeyObserver() override = default;

  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_host, content::RenderFrameHost* new_host) override;

private:
  content::WebContents* web_contents_;

};

#endif  // SEYYAH_PAGE_KEY_OBSERVER_H_
