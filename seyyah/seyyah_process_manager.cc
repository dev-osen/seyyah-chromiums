#include "seyyah_process_manager.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"



// ORNEK KULLANIM
// ./out/Default/chrome --seyyah-pid="54321"
// std::string seyyah_pid = SeyyahProcessManager::Instance().GetSeyyahPid();
// LOG(INFO) << "Current Seyyah PID: " << seyyah_pid;

const char kSeyyahPid[] = "seyyah-pid";


SeyyahProcessManager& SeyyahProcessManager::Instance() {
  static SeyyahProcessManager instance;
  return instance;
}


SeyyahProcessManager::SeyyahProcessManager() : seyyah_pid_("") {}


SeyyahProcessManager::~SeyyahProcessManager() {}


void SeyyahProcessManager::SetSeyyahPid(const std::string& pid) {
  seyyah_pid_ = pid;
  LOG(INFO) << "Seyyah PID set to: " << seyyah_pid_;
}


const std::string& SeyyahProcessManager::GetSeyyahPid() {

  if (seyyah_pid_.empty()) {
    auto* command_line = base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch(kSeyyahPid)) {
      seyyah_pid_ = command_line->GetSwitchValueASCII(kSeyyahPid);
      LOG(INFO) << "[SEYYAH][PID] Start:" << seyyah_pid_;
    } else {

      seyyah_pid_ = GenerateRandomPid();
      LOG(INFO) << "[SEYYAH][PID] Random:" << seyyah_pid_;
    }
  }
  return seyyah_pid_;
}


std::string SeyyahProcessManager::GenerateRandomPid() const {
  const int pid_length = 16;
  std::string random_pid;
  for (int i = 0; i < pid_length; ++i) {
    int random_digit = base::RandInt(0, 9);  // 0-9 arasında rastgele bir sayı.
    random_pid += base::NumberToString(random_digit);
  }
  return random_pid;
}

