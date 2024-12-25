#ifndef SEYYAH_PAGE_KEY_MANAGER_H
#define SEYYAH_PAGE_KEY_MANAGER_H

#include <string>
#include <mutex>

class SeyyahPageKeyManager {
public:

  static SeyyahPageKeyManager& Instance();

  void SetRoutingKey(int routingID, const std::string& pageKey);
  std::string GetRoutingKey(int routingID);
  static std::string GetRandomKey();

private:

  std::mutex fileMutex;

  SeyyahPageKeyManager() = default;
  ~SeyyahPageKeyManager() = default;

  SeyyahPageKeyManager(const SeyyahPageKeyManager&) = delete;
  SeyyahPageKeyManager& operator=(const SeyyahPageKeyManager&) = delete;

  static void EnsureDirectoryExists();
  static std::string GetFilePath(int routingID);
};

#endif //SEYYAH_PAGE_KEY_MANAGER_H
