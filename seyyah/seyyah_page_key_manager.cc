#include "seyyah_page_key_manager.h"
#include "seyyah_process_manager.h"
#include "seyyah_system.h"
#include "base/logging.h"
#include <random>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>


SeyyahPageKeyManager& SeyyahPageKeyManager::Instance() {
    static SeyyahPageKeyManager instance;
    return instance;
}


void SeyyahPageKeyManager::SetRoutingKey(int routingID, const std::string& pageKey) {
    std::lock_guard<std::mutex> lock(fileMutex);

    EnsureDirectoryExists();
    std::string filePath = GetFilePath(routingID);
    std::ofstream outFile(filePath, std::ios::trunc);
    if (!outFile) {
        LOG(ERROR) << "[SEYYAH][SetRoutingKey] FATAL-ERROR: Dosyaya yazamadi" << filePath;
        return;
    }
    outFile << pageKey;
    outFile.close();

//    std::cout << "SetRoutingKey - Written to file: " << filePath << ", PageKey: " << pageKey << std::endl;
}


std::string SeyyahPageKeyManager::GetRoutingKey(int routingID) {
  std::lock_guard<std::mutex> lock(fileMutex);

  std::string filePath = GetFilePath(routingID);
  std::ifstream inFile;
  const int maxRetries = 10; // 10 deneme
  int attempt = 0;

  while (attempt < maxRetries) {
    inFile.open(filePath);
    if (inFile) {
      break; // Dosya açıldıysa döngüden çık
    }

    LOG(ERROR) << "[SEYYAH][GetRoutingKey] WARN: Dosya bulunamadi! / DOSYA: " << filePath << " / DENEME: " << (attempt + 1);

    // 1 saniye bekle
    std::this_thread::sleep_for(std::chrono::seconds(1));
    attempt++;
  }

  if (!inFile) {
    LOG(ERROR) << "[SEYYAH][GetRoutingKey] FATAL-ERROR: Dosya bulunamadi! / DOSYA: " << filePath << " / DENEME: " << (maxRetries + 1);
    return "";
  }

  std::string pageKey;
  std::getline(inFile, pageKey);
  inFile.close();

  LOG(ERROR) << "[SEYYAH][GetRoutingKey] SPK:" << pageKey;

  return pageKey;
}


std::string SeyyahPageKeyManager::GetRandomKey() {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static std::default_random_engine engine(std::random_device{}());
    static std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);

    std::string randomKey;
    for (size_t i = 0; i < 16; ++i) {
        randomKey += charset[dist(engine)];
    }

    return SeyyahSystem::GetSeyyahLicense() + "." + randomKey;
}


void SeyyahPageKeyManager::EnsureDirectoryExists() {
    if (!std::filesystem::exists(SeyyahSystem::GetSeyyahServer())) {
        std::filesystem::create_directories(SeyyahSystem::GetSeyyahServer());
        std::cout << "EnsureDirectoryExists - Directory created: " << SeyyahSystem::GetSeyyahServer() << std::endl;
    }
}


std::string SeyyahPageKeyManager::GetFilePath(int routingID) {
    return SeyyahSystem::GetSeyyahServer() + "/" + SeyyahProcessManager::Instance().GetSeyyahPid() + "." + std::to_string(routingID) + ".sp";
}
