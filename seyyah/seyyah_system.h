#ifndef SEYYAH_SYSTEM_H
#define SEYYAH_SYSTEM_H

#include <string>
#include <mutex>

class SeyyahSystem {
public:

  static void SetSeyyahServer(const std::string& server);
  static std::string GetSeyyahServer();

  static void SetSeyyahLicense(const std::string& license);
  static std::string GetSeyyahLicense();

  static void SetSeyyahWorkpath(const std::string& workpath);
  static std::string GetSeyyahWorkpath();

private:

  static std::string seyyah_server_;
  static std::string seyyah_license_;
  static std::string seyyah_workpath_;

  static std::mutex mutex_;
};

#endif // SEYYAH_SYSTEM_H
