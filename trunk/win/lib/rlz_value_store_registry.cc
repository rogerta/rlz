// Copyright 2012 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.

#include "rlz/win/lib/rlz_value_store_registry.h"

#include "base/lazy_instance.h"
#include "rlz/lib/assert.h"
#include "rlz/lib/lib_values.h"
#include "rlz/win/lib/user_key.h"

namespace rlz_lib {

bool RlzValueStoreRegistry::HasAccess(AccessType type) {
  UserKey user_key;
  return user_key.HasAccess(type == kWriteAccess);
}

bool RlzValueStoreRegistry::ReadPingTime(Product product, int64* time) {
  base::win::RegKey key;
  return GetPingTimesRegKey(KEY_READ, &key) &&
      key.ReadInt64(GetProductName(product), time) == ERROR_SUCCESS;
}

bool RlzValueStoreRegistry::WritePingTime(Product product, int64 time) {
  base::win::RegKey key;
  return GetPingTimesRegKey(KEY_WRITE, &key) &&
      key.WriteValue(GetProductName(product), &time, sizeof(time),
                     REG_QWORD) == ERROR_SUCCESS;
}

bool RlzValueStoreRegistry::ClearPingTime(Product product) {
  base::win::RegKey key;
  GetPingTimesRegKey(KEY_WRITE, &key);

  const wchar_t* value_name = GetProductName(product);
  key.DeleteValue(value_name);

  // Verify deletion.
  uint64 value;
  DWORD size = sizeof(value);
  if (key.ReadValue(value_name, &value, &size, NULL) == ERROR_SUCCESS) {
    ASSERT_STRING("RlzValueStoreRegistry::ClearPingTime: Failed to delete.");
    return false;
  }

  return true;
}

ScopedRlzValueStoreLock::ScopedRlzValueStoreLock() {
  if (!lock_.failed())
    store_.reset(new RlzValueStoreRegistry);
}

ScopedRlzValueStoreLock::~ScopedRlzValueStoreLock() {
}

RlzValueStore* ScopedRlzValueStoreLock::GetStore() {
  return store_.get();
}

}  // namespace rlz_lib
