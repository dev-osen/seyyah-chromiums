#ifndef SEYYAH_RENDER_OBSERVER_MANAGER_H_
#define SEYYAH_RENDER_OBSERVER_MANAGER_H_

#include "content/public/renderer/render_frame_observer.h"
#include <set>

// Observer'ların yaşam döngüsünü yöneten singleton sınıf
class SeyyahRenderObserverManager {
public:
  // Singleton örneği almak için
  static SeyyahRenderObserverManager& Instance();

  // Observer ekleme
  void AddObserver(content::RenderFrameObserver* observer);

  // Observer kaldırma
  void RemoveObserver(content::RenderFrameObserver* observer);

  // Tüm observer'ları temizle
  void ClearObservers();

private:
  // Yapıcı ve yıkıcılar
  SeyyahRenderObserverManager();
  ~SeyyahRenderObserverManager();

  // Observer kümesi
  std::set<content::RenderFrameObserver*> observers_;
};

#endif // SEYYAH_RENDER_OBSERVER_MANAGER_H_
