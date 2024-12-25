#ifndef SEYYAH_PROCESS_MANAGER_H_
#define SEYYAH_PROCESS_MANAGER_H_

#include <string>

class SeyyahProcessManager {
public:

  static SeyyahProcessManager& Instance();

  void SetSeyyahPid(const std::string& pid);

  const std::string& GetSeyyahPid();

private:

  SeyyahProcessManager();
  ~SeyyahProcessManager();


  SeyyahProcessManager(const SeyyahProcessManager&) = delete;
  SeyyahProcessManager& operator=(const SeyyahProcessManager&) = delete;


  std::string GenerateRandomPid() const;

  std::string seyyah_pid_;
};

#endif  // SEYYAH_PROCESS_MANAGER_H_
