#ifndef SEYYAH_V8_MANIPULATOR_H
#define SEYYAH_V8_MANIPULATOR_H


#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <mutex>
#include <memory>
#include "v8/include/v8.h"

class SeyyahV8Manipulator {
public:

  // Bu metot statik olarak butun isi halleder
  static void Run(v8::Isolate* isolate, v8::Local<v8::Context> context, const std::string& page_key);

  static void SeyyahV8Manipulator::CreateFromServer(v8::Isolate* isolate,
                                           v8::Local<v8::Context> context,
                                           const std::string& page_key,
                                           std::function<void(std::shared_ptr<SeyyahV8Manipulator>)> callback) ;



  SeyyahV8Manipulator(v8::Isolate* isolate, v8::Local<v8::Context> context, const nlohmann::json& jsonFingerprint, const std::string& page_key);
  ~SeyyahV8Manipulator();
  void Protect();
  const std::string& GetPageKey() const{ return page_key_; }


private:
  v8::Isolate* isolate_;
  v8::Global<v8::Context> context_;
  nlohmann::json fingerprintData_;
  const std::string& page_key_;

  void SetPageKey();

  // Yardımcı Metodlar
  v8::Local<v8::String> GetV8String(const std::string& value);
  v8::Local<v8::Integer> GetV8Integer(int value);
  v8::Local<v8::Number> GetV8Number(double value);
  v8::Local<v8::Boolean> GetV8Boolean(bool value);
  v8::Local<v8::Value> GetV8Value(const nlohmann::json& value);

  // Override Metodları
  void OverrideNavigator();
  void OverrideScreen();
  void OverrideCanvas();
  void OverrideWebGL();
  void OverrideAudioContext();
  void OverrideFonts();
  void OverrideBatteryStatus();
  void OverrideNetworkInformation();
  void OverrideWebRTC();
  void OverrideDeviceOrientation();
  void OverrideMediaDevices();
  void OverrideStorage();
  void OverridePerformance();
  void OverridePermissions();
  void OverrideReferrer();
  void OverrideExtensions();
  void OverrideTouchSupport();
  void OverrideMediaCapabilities();
  void OverrideStorageEstimates();
  void OverrideSVGRendering();
  void OverridePageInteractions();
  void OverrideComputedStyles();
  void OverrideColorScheme();

  // Önbellek için statik üye
  static std::unordered_map<std::string, nlohmann::json> cache_;
  static std::mutex cache_mutex_;

  // Networking işlemleri
  static void OnURLLoadComplete(std::shared_ptr<SeyyahV8Manipulator> protector,
                                            std::unique_ptr<network::SimpleURLLoader> loader,
                                            std::function<void(std::shared_ptr<SeyyahV8Manipulator>)> callback,
                                            std::unique_ptr<std::string> response_body);

  // SimpleURLLoader instance'ını saklamak için
  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  // SimpleURLLoader::BodyCallback implementasyonu
  void OnComplete(network::SimpleURLLoader* loader, std::unique_ptr<std::string> response_body) override;
};





#endif //SEYYAH_V8_MANIPULATOR_H
