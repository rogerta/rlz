// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A helper library to keep track of a user's key by SID.
// Used by RLZ libary. Also to be used by SearchWithGoogle library.

#include "rlz/win/lib/user_key.h"

#include "rlz/win/lib/assert.h"
#include "rlz/win/lib/process_info.h"

namespace rlz_lib {

UserKey::UserKey(const wchar_t* sid) {
  if (!sid || !sid[0]) {
    if (!user_key_.Open(HKEY_CURRENT_USER, L""))
      ASSERT_STRING("Could not open HKEY_CURRENT_USER");
    return;
  }

  // Disallow impersonation when not running as administrator / high integrity.
  if (!ProcessInfo::HasAdminRights()) {
    ASSERT_STRING("UserKey::UserKey Cannot set SID when not administrator.");
    return;
  }

  if (!user_key_.Open(HKEY_USERS, sid, KEY_ALL_ACCESS))
    ASSERT_STRING("UserKey::UserKey Failed to open user key.");
}

HKEY UserKey::Get() {
  CHECK(user_key_.Valid());
  return user_key_.Handle();
}

bool UserKey::HasAccess(bool write_access) {
  return HasAccess(user_key_.Handle(), write_access);
}

bool UserKey::HasAccess(HKEY user_key, bool write_access) {
  if (!user_key) {
    ASSERT_STRING("UserKey::HasAccess: No key opened.");
    return false;
  }

  if (ProcessInfo::IsRunningAsSystem()) {
    if (user_key == HKEY_CURRENT_USER) {
      ASSERT_STRING("UserKey::HasAccess: No access as SYSTEM without SID set.");
      return false;
    }
    return true;
  }

  if (write_access) {
    if (!ProcessInfo::IsVistaOrLater()) return true;
    ProcessInfo::IntegrityLevel level = ProcessInfo::INTEGRITY_UNKNOWN;
    if (!ProcessInfo::GetIntegrityLevel(&level)) {
      ASSERT_STRING("UserKey::HasAccess: Cannot determine Integrity Level.");
      return false;
    }
    if (level <= ProcessInfo::LOW_INTEGRITY) {
      ASSERT_STRING("UserKey::HasAccess: Cannot write from Low Integrity.");
      return false;
    }
  }
  return true;
}

}  // namespace rlz_lib
