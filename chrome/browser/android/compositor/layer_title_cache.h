// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_COMPOSITOR_LAYER_TITLE_CACHE_H_
#define CHROME_BROWSER_ANDROID_COMPOSITOR_LAYER_TITLE_CACHE_H_

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/containers/id_map.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "cc/resources/ui_resource_client.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/transform.h"

namespace cc::slim {
class Layer;
}

namespace ui {
class ResourceManager;
}

namespace android {

class DecorationTabTitle;
class DecorationIconTitle;

// A native component of the Java LayerTitleCache class.  This class
// will build and maintain layers that represent the cached titles in
// the Java class.
class LayerTitleCache {
 public:
  static LayerTitleCache* FromJavaObject(
      const base::android::JavaRef<jobject>& jobj);

  LayerTitleCache(JNIEnv* env,
                  const jni_zero::JavaRef<jobject>& obj,
                  jint fade_width,
                  jint icon_start_padding,
                  jint icon_end_padding,
                  jint spinner_resource_id,
                  jint spinner_incognito_resource_id,
                  ui::ResourceManager* resource_manager);

  LayerTitleCache(const LayerTitleCache&) = delete;
  LayerTitleCache& operator=(const LayerTitleCache&) = delete;

  void Destroy(JNIEnv* env);

  // Called from Java, updates a native cc::slim::Layer based on the new texture
  // information.
  void UpdateLayer(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& obj,
                   jint tab_id,
                   jint title_resource_id,
                   jint icon_resource_id,
                   bool is_incognito,
                   bool is_rtl);

  // Called from Java, updates a native cc::slim::Layer based on the new texture
  // information.
  void UpdateGroupLayer(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj,
                        jint group_root_id,
                        jint title_resource_id,
                        jint avatar_resource_id,
                        jint avatar_padding,
                        bool is_incognito,
                        bool is_rtl);

  // Called from Java, updates icon.
  void UpdateIcon(JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& obj,
                  jint tab_id,
                  jint icon_resource_id);

  // Returns the layer that represents the title of tab of tab_id.
  // Returns NULL if no layer can be found.
  DecorationTabTitle* GetTitleLayer(int tab_id);

  // Returns the layer that represents the title of group of group_root_id.
  // Returns NULL if no layer can be found.
  DecorationIconTitle* GetGroupTitleLayer(int group_root_id, bool incognito);

 private:
  const int kEmptyWidth = 0;

  virtual ~LayerTitleCache();

  base::IDMap<std::unique_ptr<DecorationTabTitle>> layer_cache_;
  base::IDMap<std::unique_ptr<DecorationIconTitle>> group_layer_cache_;

  JavaObjectWeakGlobalRef weak_java_title_cache_;
  int fade_width_;
  int icon_start_padding_;
  int icon_end_padding_;

  int spinner_resource_id_;
  int spinner_incognito_resource_id_;

  raw_ptr<ui::ResourceManager> resource_manager_;
};

}  // namespace android

#endif  // CHROME_BROWSER_ANDROID_COMPOSITOR_LAYER_TITLE_CACHE_H_
