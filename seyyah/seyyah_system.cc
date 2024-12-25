#include "seyyah_system.h"

// Tanımlamalar
std::string SeyyahSystem::seyyah_server_ = "http://localhost:3540";
std::string SeyyahSystem::seyyah_license_ = "BVTec6CwAHFMYR24uXD9yj";
std::string SeyyahSystem::seyyah_workpath_ = "/seyyah/chromium";

std::mutex SeyyahSystem::mutex_;


void SeyyahSystem::SetSeyyahServer(const std::string& server) {
  std::lock_guard<std::mutex> lock(mutex_);
  seyyah_server_ = server;
}

std::string SeyyahSystem::GetSeyyahServer() {
  std::lock_guard<std::mutex> lock(mutex_);
  return seyyah_server_;
}


void SeyyahSystem::SetSeyyahLicense(const std::string& license) {
  std::lock_guard<std::mutex> lock(mutex_);
  seyyah_license_ = license;
}

std::string SeyyahSystem::GetSeyyahLicense() {
  std::lock_guard<std::mutex> lock(mutex_);
  return seyyah_license_;
}


void SeyyahSystem::SetSeyyahWorkpath(const std::string& workpath) {
  std::lock_guard<std::mutex> lock(mutex_);
  seyyah_workpath_ = workpath;
}

std::string SeyyahSystem::GetSeyyahWorkpath() {
  std::lock_guard<std::mutex> lock(mutex_);
  return seyyah_workpath_;
}
