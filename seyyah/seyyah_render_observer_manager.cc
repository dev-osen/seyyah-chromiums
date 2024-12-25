#include "seyyah_render_observer_manager.h"

SeyyahRenderObserverManager& SeyyahRenderObserverManager::Instance() {
  static SeyyahRenderObserverManager instance;
  return instance;
}

SeyyahRenderObserverManager::SeyyahRenderObserverManager() = default;

SeyyahRenderObserverManager::~SeyyahRenderObserverManager() = default;

void SeyyahRenderObserverManager::AddObserver(content::RenderFrameObserver* observer) {
  observers_.insert(observer);
}

void SeyyahRenderObserverManager::RemoveObserver(content::RenderFrameObserver* observer) {
  observers_.erase(observer);
}

void SeyyahRenderObserverManager::ClearObservers() {
  observers_.clear();
}
