// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A helper library to keep track of a user's key by SID.
// Used by RLZ libary. Also to be used by SearchWithGoogle library.

#include "rlz/win/lib/user_key.h"

#include "base/process_util.h"
#include "base/win/windows_version.h"

#include "rlz/win/lib/assert.h"
#include "rlz/win/lib/process_info.h"

namespace rlz_lib {

UserKey::UserKey(const wchar_t* sid) {
  if (!sid || !sid[0]) {
    // No SID is specified, so the caller is trying to access HKEY_CURRENT_USER.
    // Test to see if we can read from there.  Don't try HKEY_CURRENT_USER
    // because this will cause problems in the unit tests: if we open
    // HKEY_CURRENT_USER directly here, the overriding done for unit tests will
    // no longer work.  So we try subkey "Software" which is known to always
    // exist.
    base::win::RegKey key;
    if (key.Open(HKEY_CURRENT_USER, L"Software", KEY_READ) != ERROR_SUCCESS)
      ASSERT_STRING("Could not open HKEY_CURRENT_USER");
    return;
  }

  // Disallow impersonation when not running as administrator / high integrity.
  if (!ProcessInfo::HasAdminRights()) {
    ASSERT_STRING("UserKey::UserKey Cannot set SID when not administrator.");
    return;
  }

  if (user_key_.Open(HKEY_USERS, sid, KEY_ALL_ACCESS) != ERROR_SUCCESS)
    ASSERT_STRING("UserKey::UserKey Failed to open user key.");
}

HKEY UserKey::Get() {
  // If user_key_ is not valid, this is because the caller is trying to access
  // HKEY_CURRENT_USER.
  return user_key_.Valid() ? user_key_.Handle() : HKEY_CURRENT_USER;
}

bool UserKey::HasAccess(bool write_access) {
  return HasAccess(Get(), write_access);
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
    if (base::win::GetVersion() < base::win::VERSION_VISTA) return true;
    base::ProcessHandle process_handle = base::GetCurrentProcessHandle();
    base::IntegrityLevel level = base::INTEGRITY_UNKNOWN;

    if (!base::GetProcessIntegrityLevel(process_handle, &level)) {
      ASSERT_STRING("UserKey::HasAccess: Cannot determine Integrity Level.");
      return false;
    }
    if (level <= base::LOW_INTEGRITY) {
      ASSERT_STRING("UserKey::HasAccess: Cannot write from Low Integrity.");
      return false;
    }
  }
  return true;
}

}  // namespace rlz_lib
