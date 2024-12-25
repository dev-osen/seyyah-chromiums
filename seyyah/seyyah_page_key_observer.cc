#include "seyyah_page_key_observer.h"
#include "seyyah_page_key_manager.h"
#include "base/strings/utf_string_conversions.h"
#include "base/logging.h"


SeyyahPageKeyObserver::SeyyahPageKeyObserver(content::WebContents* web_contents)
    : WebContentsObserver(web_contents), web_contents_(web_contents) {}

void SeyyahPageKeyObserver::RenderFrameCreated(content::RenderFrameHost* render_frame_host) {
    const std::string* spk = web_contents_->GetSeyyahPageKey();
    SeyyahPageKeyManager::Instance().SetRoutingKey(render_frame_host->GetRoutingID(), *spk);
}

void SeyyahPageKeyObserver::RenderFrameHostChanged(content::RenderFrameHost* old_host, content::RenderFrameHost* new_host) {
  if (new_host) {
    const std::string* spk = web_contents_->GetSeyyahPageKey();
    SeyyahPageKeyManager::Instance().SetRoutingKey(new_host->GetRoutingID(), *spk);
  }
}
