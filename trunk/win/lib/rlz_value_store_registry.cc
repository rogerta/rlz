// Copyright 2012 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.

#include "rlz/win/lib/rlz_value_store_registry.h"

#include "base/utf_string_conversions.h"
#include "rlz/lib/assert.h"
#include "rlz/lib/lib_values.h"
#include "rlz/lib/string_utils.h"
#include "rlz/win/lib/user_key.h"

namespace rlz_lib {

bool RlzValueStoreRegistry::HasAccess(AccessType type) {
  UserKey user_key;
  return user_key.HasAccess(type == kWriteAccess);
}

bool RlzValueStoreRegistry::WritePingTime(Product product, int64 time) {
  base::win::RegKey key;
  return GetPingTimesRegKey(KEY_WRITE, &key) &&
      key.WriteValue(GetProductName(product), &time, sizeof(time),
                     REG_QWORD) == ERROR_SUCCESS;
}

bool RlzValueStoreRegistry::ReadPingTime(Product product, int64* time) {
  base::win::RegKey key;
  return GetPingTimesRegKey(KEY_READ, &key) &&
      key.ReadInt64(GetProductName(product), time) == ERROR_SUCCESS;
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

bool RlzValueStoreRegistry::WriteAccessPointRlz(AccessPoint access_point,
                                                const char* new_rlz) {
  const char* access_point_name = GetAccessPointName(access_point);
  if (!access_point_name)
    return false;

  std::wstring access_point_name_wide(ASCIIToWide(access_point_name));
  base::win::RegKey key;
  GetAccessPointRlzsRegKey(KEY_WRITE, &key);

  if (!RegKeyWriteValue(key, access_point_name_wide.c_str(), new_rlz)) {
    ASSERT_STRING("SetAccessPointRlz: Could not write the new RLZ value");
    return false;
  }
  return true;
}

bool RlzValueStoreRegistry::ReadAccessPointRlz(AccessPoint access_point,
                                               char* rlz,
                                               size_t rlz_size) {
  const char* access_point_name = GetAccessPointName(access_point);
  if (!access_point_name)
    return false;

  size_t size = rlz_size;
  base::win::RegKey key;
  GetAccessPointRlzsRegKey(KEY_READ, &key);
  if (!RegKeyReadValue(key, ASCIIToWide(access_point_name).c_str(),
                       rlz, &size)) {
    rlz[0] = 0;
    if (size > rlz_size) {
      ASSERT_STRING("GetAccessPointRlz: Insufficient buffer size");
      return false;
    }
  }
  return true;
}

bool RlzValueStoreRegistry::ClearAccessPointRlz(AccessPoint access_point) {
  const char* access_point_name = GetAccessPointName(access_point);
  if (!access_point_name)
    return false;

  std::wstring access_point_name_wide(ASCIIToWide(access_point_name));
  base::win::RegKey key;
  GetAccessPointRlzsRegKey(KEY_WRITE, &key);

  key.DeleteValue(access_point_name_wide.c_str());

  // Verify deletion.
  DWORD value;
  if (key.ReadValueDW(access_point_name_wide.c_str(), &value) ==
      ERROR_SUCCESS) {
    ASSERT_STRING("SetAccessPointRlz: Could not clear the RLZ value.");
    return false;
  }
  return true;
}

bool RlzValueStoreRegistry::AddProductEvent(Product product,
                                            const char* event_rlz) {
  std::wstring event_rlz_wide(ASCIIToWide(event_rlz));
  DWORD value = 1;
  base::win::RegKey reg_key;
  rlz_lib::GetEventsRegKey(kEventsSubkeyName, &product,
                           KEY_WRITE, &reg_key);
  if (reg_key.WriteValue(event_rlz_wide.c_str(), value) != ERROR_SUCCESS) {
    ASSERT_STRING("AddProductEvent: Could not write the new event value");
    return false;
  }

  return true;
}

bool RlzValueStoreRegistry::AddStatefulEvent(Product product,
                                             const char* event_rlz) {
  DWORD data = 1;

  base::win::RegKey key;
  std::wstring event_rlz_wide(ASCIIToWide(event_rlz));
  if (!GetEventsRegKey(rlz_lib::kStatefulEventsSubkeyName,
                       &product, KEY_WRITE, &key) ||
      key.WriteValue(event_rlz_wide.c_str(), data) != ERROR_SUCCESS) {
    ASSERT_STRING(
        "AddStatefulEvent: Could not write the new stateful event");
    return false;
  }

  return true;
}

bool RlzValueStoreRegistry::IsStatefulEvent(Product product,
                                            const char* event_rlz) {
  DWORD value;
  base::win::RegKey key;
  rlz_lib::GetEventsRegKey(kStatefulEventsSubkeyName, &product,
                           KEY_READ, &key);
  std::wstring event_rlz_wide(ASCIIToWide(event_rlz));
  return key.ReadValueDW(event_rlz_wide.c_str(), &value) == ERROR_SUCCESS;
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
