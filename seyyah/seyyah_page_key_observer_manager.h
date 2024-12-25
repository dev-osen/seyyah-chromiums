#ifndef SEYYAH_PAGE_KEY_OBSERVER_MANAGER_H_
#define SEYYAH_PAGE_KEY_OBSERVER_MANAGER_H_

#include "seyyah_page_key_observer.h"
#include "content/public/browser/web_contents.h"
#include <map>
#include <memory>

class SeyyahPageKeyObserverManager {
public:
  static SeyyahPageKeyObserverManager& Instance();

  void AddObserver(content::WebContents* web_contents);
  void RemoveObserver(content::WebContents* web_contents);

private:
  SeyyahPageKeyObserverManager() = default;
  ~SeyyahPageKeyObserverManager() = default;

  std::map<content::WebContents*, std::unique_ptr<SeyyahPageKeyObserver>> observers_;
};

#endif  // SEYYAH_PAGE_KEY_OBSERVER_MANAGER_H_
