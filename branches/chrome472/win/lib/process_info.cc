// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Information about the current process.

#include "rlz/win/lib/process_info.h"

#include <windows.h>
#include <Sddl.h>  // For ConvertSidToStringSid.
#include <LMCons.h>  // For UNLEN

#include "base/logging.h"
#include "base/scoped_handle_win.h"
#include "base/scoped_ptr.h"
#include "rlz/win/lib/assert.h"
#include "rlz/win/lib/vista_winnt.h"

namespace {

HRESULT GetCurrentUser(std::wstring* name,
                       std::wstring* domain,
                       std::wstring* sid) {
  DWORD err;

  // Get the current username & domain the hard way.  (GetUserNameEx would be
  // nice, but unfortunately requires connectivity to a domain controller.
  // Useless.)

  // (Following call doesn't work if running as a Service - because a Service
  // runs under special accounts like LOCAL_SYSTEM, not as the logged in user.
  // In which case, search for and use the process handle of a running
  // Explorer.exe.)
  HANDLE token;
  if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token))
    return E_FAIL;

  ScopedHandle scoped_process_token(token);

  // (Following call will fail with ERROR_INSUFFICIENT_BUFFER and give us the
  // required size.)
  scoped_array<char> token_user_bytes;
  DWORD token_user_size;
  DWORD token_user_size2;
  BOOL result = ::GetTokenInformation(token, TokenUser, NULL, 0,
                                      &token_user_size);
  err = ::GetLastError();
  CHECK(!result && err == ERROR_INSUFFICIENT_BUFFER);

  token_user_bytes.reset(new char[token_user_size]);
  if (!token_user_bytes.get())
    return E_OUTOFMEMORY;

  if (!::GetTokenInformation(token, TokenUser, token_user_bytes.get(),
                             token_user_size, &token_user_size2)) {
    return E_FAIL;
  }

  WCHAR user_name[UNLEN + 1];  // max username length
  WCHAR domain_name[UNLEN + 1];
  DWORD user_name_size = UNLEN + 1;
  DWORD domain_name_size = UNLEN + 1;
  SID_NAME_USE sid_type;
  TOKEN_USER* token_user =
      reinterpret_cast<TOKEN_USER*>(token_user_bytes.get());
  if (!token_user)
    return E_FAIL;
  PSID user_sid = token_user->User.Sid;
  if (!::LookupAccountSidW(NULL, user_sid, user_name, &user_name_size,
                           domain_name, &domain_name_size, &sid_type)) {
    return E_FAIL;
  }

  if (name != NULL) {
    *name = user_name;
  }
  if (domain != NULL) {
    *domain = domain_name;
  }
  if (sid != NULL) {
    LPWSTR string_sid;
    ConvertSidToStringSidW(user_sid, &string_sid);
    *sid = string_sid;  // copy out to cstring
    // free memory, as documented for ConvertSidToStringSid
    LocalFree(string_sid);
  }

  return S_OK;
}

HRESULT GetElevationType(PTOKEN_ELEVATION_TYPE elevation) {
  if (!elevation)
    return E_POINTER;

  *elevation = TokenElevationTypeDefault;

  if (!rlz_lib::ProcessInfo::IsVistaOrLater())
    return E_FAIL;

  HANDLE process_token;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &process_token))
    return HRESULT_FROM_WIN32(GetLastError());

  ScopedHandle scoped_process_token(process_token);

  DWORD size;
  TOKEN_ELEVATION_TYPE elevation_type;
  if (!GetTokenInformation(process_token, TokenElevationType, &elevation_type,
                           sizeof(elevation_type), &size)) {
    return HRESULT_FROM_WIN32(GetLastError());
  }

  *elevation = elevation_type;
  return S_OK;
}

// based on http://msdn2.microsoft.com/en-us/library/aa376389.aspx
bool GetUserGroup(long* group) {
  if (!group)
    return false;

  *group = 0;

  // groups are listed in DECREASING order of importance
  // (eg. If a user is a member of both the admin group and
  // the power user group, it is more useful to list the user
  // as an admin)
  DWORD user_groups[] =  {DOMAIN_ALIAS_RID_ADMINS,
                          DOMAIN_ALIAS_RID_POWER_USERS};
  SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;

  for (int i = 0; i < arraysize(user_groups) && *group == 0; ++i) {
    PSID current_group;
    if (AllocateAndInitializeSid(&nt_authority, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 user_groups[i], 0, 0, 0, 0,
                                 0, 0, &current_group)) {
      BOOL current_level;
      if (CheckTokenMembership(NULL, current_group, &current_level) &&
          current_level) {
        *group = user_groups[i];
      }

      FreeSid(current_group);
    }
  }

  return group != 0;
}

HRESULT GetProcessIntegrityLevel(HANDLE process,
                                 rlz_lib::ProcessInfo::IntegrityLevel *level) {
  if (!level)
    return E_POINTER;

  *level = rlz_lib::ProcessInfo::INTEGRITY_UNKNOWN;

  if (!rlz_lib::ProcessInfo::IsVistaOrLater())
    return E_FAIL;

  HANDLE process_token;
  if (!OpenProcessToken(process, TOKEN_QUERY | TOKEN_QUERY_SOURCE,
                        &process_token)) {
    return HRESULT_FROM_WIN32(GetLastError());
  }

  ScopedHandle scoped_process_token(process_token);

  SetLastError(0);  // clear last error first
  DWORD token_info_length = 0;
  GetTokenInformation(process_token, TokenIntegrityLevel, NULL, 0,
                      &token_info_length);

  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    return HRESULT_FROM_WIN32(GetLastError());

  scoped_array<char> token_label_bytes(new char[token_info_length]);
  if (!token_label_bytes.get())
    return E_OUTOFMEMORY;

  TOKEN_MANDATORY_LABEL* token_label =
      reinterpret_cast<TOKEN_MANDATORY_LABEL*>(token_label_bytes.get());
  if (!token_label)
    return E_OUTOFMEMORY;

  if (!GetTokenInformation(process_token, TokenIntegrityLevel,
                           token_label, token_info_length, &token_info_length))
    return HRESULT_FROM_WIN32(GetLastError());

  DWORD integrity_level = *GetSidSubAuthority(token_label->Label.Sid,
      (DWORD)(UCHAR)(*GetSidSubAuthorityCount(token_label->Label.Sid)-1));

  if (integrity_level < SECURITY_MANDATORY_MEDIUM_RID) {
    *level = rlz_lib::ProcessInfo::LOW_INTEGRITY;
  } else if (integrity_level >= SECURITY_MANDATORY_MEDIUM_RID &&
      integrity_level < SECURITY_MANDATORY_HIGH_RID) {
    *level = rlz_lib::ProcessInfo::MEDIUM_INTEGRITY;
  } else if (integrity_level >= SECURITY_MANDATORY_HIGH_RID) {
    *level = rlz_lib::ProcessInfo::HIGH_INTEGRITY;
  }

  return S_OK;
}

}  //anonymous


namespace rlz_lib {

bool ProcessInfo::GetIntegrityLevel(IntegrityLevel *level) {
  if (!level)
    return false;

  *level = INTEGRITY_UNKNOWN;

  if (!IsVistaOrLater())
    return false;

  static bool integrity_level_set = false;
  static IntegrityLevel process_integrity_level;
  if (!integrity_level_set) {
    if (FAILED(GetProcessIntegrityLevel(GetCurrentProcess(),
                                        &process_integrity_level))) {
      ASSERT_STRING("GetProcessIntegrityLevel failed");
      return false;
    }

    integrity_level_set = true;
  }

  *level = process_integrity_level;
  return true;
}

bool ProcessInfo::IsRunningAsSystem() {
  static std::wstring name;
  static std::wstring domain;
  static std::wstring sid;
  if (name.empty())
    CHECK(SUCCEEDED(GetCurrentUser(&name, &domain, &sid)));

  return (name == L"SYSTEM");
}

bool ProcessInfo::IsVistaOrLater() {
  static bool known = false;
  static bool is_vista = false;
  if (!known) {
    OSVERSIONINFOEX osvi = { 0 };
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = 6;
    DWORDLONG conditional = 0;
    VER_SET_CONDITION(conditional, VER_MAJORVERSION, VER_GREATER_EQUAL);
    is_vista = !!VerifyVersionInfo(&osvi, VER_MAJORVERSION, conditional);
    known = true;
  }

  return is_vista;
}

bool ProcessInfo::HasAdminRights() {
  static bool evaluated = false;
  static bool has_rights = false;

  if (!evaluated) {
    if (IsRunningAsSystem()) {
      has_rights = true;
    } else if (IsVistaOrLater()) {
      TOKEN_ELEVATION_TYPE elevation;
      IntegrityLevel level;
      if (SUCCEEDED(GetElevationType(&elevation)) && GetIntegrityLevel(&level))
        has_rights = (elevation == TokenElevationTypeFull) ||
                    (level == HIGH_INTEGRITY);
    } else {
      long group = 0;
      if (GetUserGroup(&group))
        has_rights = (group == DOMAIN_ALIAS_RID_ADMINS);
    }
  }

  evaluated = true;
  if (!has_rights)
    ASSERT_STRING("ProcessInfo::HasAdminRights: Does not have admin rights.");

  return has_rights;
}

};  // namespace
