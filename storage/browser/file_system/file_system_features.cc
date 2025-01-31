// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/file_system/file_system_features.h"

namespace storage::features {

// Creates FileSystemContexts in incognito mode. This is used to run web tests
// in incognito mode to ensure feature parity for FileSystemAccessAccessHandles.
BASE_FEATURE(kIncognitoFileSystemContextForTesting,
             "IncognitoFileSystemContextForTesting",
             base::FEATURE_DISABLED_BY_DEFAULT);

// If enabled, files of FileSystemType::kSyncable will map to
// StorageType::kTemporary instead of StorageType::kSyncable, making them
// eligible for storage eviction during storage pressure.
BASE_FEATURE(kDisableSyncableQuota,
             "DisableSyncableQuota",
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace storage::features
