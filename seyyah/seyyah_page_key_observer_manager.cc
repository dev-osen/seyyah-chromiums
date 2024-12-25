#include "seyyah_page_key_observer_manager.h"
#include "base/logging.h"

SeyyahPageKeyObserverManager& SeyyahPageKeyObserverManager::Instance() {
  static SeyyahPageKeyObserverManager instance;
  return instance;
}

void SeyyahPageKeyObserverManager::AddObserver(content::WebContents* web_contents) {
  if (observers_.find(web_contents) != observers_.end()) {
    return;
  }
  observers_[web_contents] = std::make_unique<SeyyahPageKeyObserver>(web_contents);
}

void SeyyahPageKeyObserverManager::RemoveObserver(content::WebContents* web_contents) {
  observers_.erase(web_contents);
}