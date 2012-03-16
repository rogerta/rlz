// Copyright 2011 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A library to manage RLZ information for access-points shared
// across different client applications.

#include "rlz/win/lib/rlz_lib.h"

#include <windows.h>
#include <aclapi.h>
#include <winbase.h>
#include <winerror.h>
#include <vector>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/utf_string_conversions.h"
#include "base/win/registry.h"
#include "base/win/windows_version.h"
#include "rlz/lib/assert.h"
#include "rlz/lib/financial_ping.h"
#include "rlz/lib/lib_values.h"
#include "rlz/lib/rlz_value_store.h"
#include "rlz/lib/string_utils.h"
#include "rlz/win/lib/lib_mutex.h"
#include "rlz/win/lib/machine_deal.h"
#include "rlz/win/lib/user_key.h"

namespace {

// Path to recursively copy into the replacemment hives.  These are needed
// to make sure certain win32 APIs continue to run correctly once the real
// hives are replaced.
const wchar_t* kHKLMAccessProviders =
    L"System\\CurrentControlSet\\Control\\Lsa\\AccessProviders";

// Helper functions

// Deletes a registry key if it exists and has no subkeys or values.
// TODO: Move this to a registry_utils file and add unittest.
bool DeleteKeyIfEmpty(HKEY root_key, const wchar_t* key_name) {
  if (!key_name) {
    ASSERT_STRING("DeleteKeyIfEmpty: key_name is NULL");
    return false;
  } else {  // Scope needed for RegKey
    base::win::RegKey key(root_key, key_name, KEY_READ);
    if (!key.Valid())
      return true;  // Key does not exist - nothing to do.

    base::win::RegistryKeyIterator key_iter(root_key, key_name);
    if (key_iter.SubkeyCount() > 0)
      return true;  // Not empty, so nothing to do

    base::win::RegistryValueIterator value_iter(root_key, key_name);
    if (value_iter.ValueCount() > 0)
      return true;  // Not empty, so nothing to do
  }

  // The key is empty - delete it now.
  base::win::RegKey key(root_key, L"", KEY_WRITE);
  return key.DeleteKey(key_name) == ERROR_SUCCESS;
}

bool GetProductEventsAsCgiHelper(rlz_lib::Product product, char* cgi,
                                 size_t cgi_size,
                                 rlz_lib::RlzValueStore* store) {
  // Prepend the CGI param key to the buffer.
  std::string cgi_arg;
  base::StringAppendF(&cgi_arg, "%s=", rlz_lib::kEventsCgiVariable);
  if (cgi_size <= cgi_arg.size())
    return false;

  size_t index;
  for (index = 0; index < cgi_arg.size(); ++index)
    cgi[index] = cgi_arg[index];

  // Read stored events.
  std::vector<std::string> events;
  if (!store->ReadProductEvents(product, &events))
    return false;

  // Append the events to the buffer.
  size_t buffer_size = cgi_size - cgi_arg.size();
  size_t num_values = 0;

  for (num_values = 0; num_values < events.size(); ++num_values) {
    cgi[index] = '\0';

    int divider = num_values > 0 ? 1 : 0;
    int size = cgi_size - (index + divider);
    if (size <= 0)
      return cgi_size >= (rlz_lib::kMaxCgiLength + 1);

    strncpy(cgi + index + divider, events[num_values].c_str(), size);
    if (divider)
      cgi[index] = rlz_lib::kEventsCgiSeparator;

    index += std::min((int)events[num_values].length(), size) + divider;
  }

  cgi[index] = '\0';

  return num_values > 0;
}

bool ClearAllProductEventValues(rlz_lib::Product product, const wchar_t* key) {
  rlz_lib::LibMutex lock;
  if (lock.failed())
    return false;

  rlz_lib::UserKey user_key;
  if (!user_key.HasAccess(true))
    return false;

  const wchar_t* product_name = rlz_lib::GetProductName(product);
  if (!product_name)
    return false;

  base::win::RegKey reg_key;
  rlz_lib::GetEventsRegKey(key, NULL, KEY_WRITE, &reg_key);
  reg_key.DeleteKey(product_name);

  // Verify that the value no longer exists.
  base::win::RegKey product_events(reg_key.Handle(), product_name, KEY_READ);
  if (product_events.Valid()) {
    ASSERT_STRING("ClearAllProductEvents: Key deletion failed");
    return false;
  }

  return true;
}

void CopyRegistryTree(const base::win::RegKey& src, base::win::RegKey* dest) {
  // First copy values.
  for (base::win::RegistryValueIterator i(src.Handle(), L"");
       i.Valid(); ++i) {
    dest->WriteValue(i.Name(), reinterpret_cast<const void*>(i.Value()),
                     i.ValueSize(), i.Type());
  }

  // Next copy subkeys recursively.
  for (base::win::RegistryKeyIterator i(src.Handle(), L"");
       i.Valid(); ++i) {
    base::win::RegKey subkey(dest->Handle(), i.Name(), KEY_ALL_ACCESS);
    CopyRegistryTree(base::win::RegKey(src.Handle(), i.Name(), KEY_READ),
                     &subkey);
  }
}

}  // namespace anonymous


namespace rlz_lib {

bool GetProductEventsAsCgi(Product product, char* cgi, size_t cgi_size) {
  if (!cgi || cgi_size <= 0) {
    ASSERT_STRING("GetProductEventsAsCgi: Invalid buffer");
    return false;
  }

  cgi[0] = 0;

  ScopedRlzValueStoreLock lock;
  RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(RlzValueStore::kReadAccess))
    return false;

  size_t size_local = std::min(
      static_cast<size_t>(kMaxCgiLength + 1), cgi_size);
  bool result = GetProductEventsAsCgiHelper(product, cgi, size_local, store);

  if (!result) {
    ASSERT_STRING("GetProductEventsAsCgi: Possibly insufficient buffer size");
    cgi[0] = 0;
    return false;
  }

  return true;
}

bool ClearAllProductEvents(Product product) {
  bool result;

  result = ClearAllProductEventValues(product, kEventsSubkeyName);
  result &= ClearAllProductEventValues(product, kStatefulEventsSubkeyName);
  return result;
}


// OEM Deal confirmation storage functions.

template<class T>
class typed_buffer_ptr {
  scoped_array<char> buffer_;

 public:
  typed_buffer_ptr() {
  }

  explicit typed_buffer_ptr(size_t size) : buffer_(new char[size]) {
  }

  void reset(size_t size) {
    buffer_.reset(new char[size]);
  }

  operator T*() {
    return reinterpret_cast<T*>(buffer_.get());
  }
};

// Check if this SID has the desired access by scanning the ACEs in the DACL.
// This function is part of the rlz_lib namespace so that it can be called from
// unit tests.  Non-unit test code should not call this function.
bool HasAccess(PSID sid, ACCESS_MASK access_mask, ACL* dacl) {
  if (dacl == NULL)
    return false;

  ACL_SIZE_INFORMATION info;
  if (!GetAclInformation(dacl, &info, sizeof(info), AclSizeInformation))
    return false;

  GENERIC_MAPPING generic_mapping = {KEY_READ, KEY_WRITE, KEY_EXECUTE,
                                     KEY_ALL_ACCESS};
  MapGenericMask(&access_mask, &generic_mapping);

  for (DWORD i = 0; i < info.AceCount; ++i) {
    ACCESS_ALLOWED_ACE* ace;
    if (GetAce(dacl, i, reinterpret_cast<void**>(&ace))) {
      if ((ace->Header.AceFlags & INHERIT_ONLY_ACE) == INHERIT_ONLY_ACE)
        continue;

      PSID existing_sid = reinterpret_cast<PSID>(&ace->SidStart);
      DWORD mask = ace->Mask;
      MapGenericMask(&mask, &generic_mapping);

      if (ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE &&
         (mask & access_mask) == access_mask && EqualSid(existing_sid, sid))
        return true;

      if (ace->Header.AceType == ACCESS_DENIED_ACE_TYPE &&
         (mask & access_mask) != 0 && EqualSid(existing_sid, sid))
        return false;
    }
  }

  return false;
}

bool CreateMachineState() {
  LibMutex lock;
  if (lock.failed())
    return false;

  base::win::RegKey hklm_key;
  if (hklm_key.Create(HKEY_LOCAL_MACHINE, kLibKeyName,
                      KEY_ALL_ACCESS | KEY_WOW64_32KEY) != ERROR_SUCCESS) {
    ASSERT_STRING("rlz_lib::CreateMachineState: "
                  "Unable to create / open machine key.");
    return false;
  }

  // Create a SID that represents ALL USERS.
  DWORD users_sid_size = SECURITY_MAX_SID_SIZE;
  typed_buffer_ptr<SID> users_sid(users_sid_size);
  CreateWellKnownSid(WinBuiltinUsersSid, NULL, users_sid, &users_sid_size);

  // Get the security descriptor for the registry key.
  DWORD original_sd_size = 0;
  ::RegGetKeySecurity(hklm_key.Handle(), DACL_SECURITY_INFORMATION, NULL,
      &original_sd_size);
  typed_buffer_ptr<SECURITY_DESCRIPTOR> original_sd(original_sd_size);

  LONG result = ::RegGetKeySecurity(hklm_key.Handle(),
      DACL_SECURITY_INFORMATION, original_sd, &original_sd_size);
  if (result != ERROR_SUCCESS) {
    ASSERT_STRING("rlz_lib::CreateMachineState: "
                  "Unable to create / open machine key.");
    return false;
  }

  // Make a copy of the security descriptor so we can modify it.  The one
  // returned by RegGetKeySecurity() is self-relative, so we need to make it
  // absolute.
  DWORD new_sd_size = 0;
  DWORD dacl_size = 0;
  DWORD sacl_size = 0;
  DWORD owner_size = 0;
  DWORD group_size = 0;
  ::MakeAbsoluteSD(original_sd, NULL, &new_sd_size, NULL, &dacl_size,
                        NULL, &sacl_size, NULL, &owner_size,
                        NULL, &group_size);

  typed_buffer_ptr<SECURITY_DESCRIPTOR> new_sd(new_sd_size);
  // Make sure the DACL is big enough to add one more ACE.
  typed_buffer_ptr<ACL> dacl(dacl_size + SECURITY_MAX_SID_SIZE);
  typed_buffer_ptr<ACL> sacl(sacl_size);
  typed_buffer_ptr<SID> owner(owner_size);
  typed_buffer_ptr<SID> group(group_size);

  if (!::MakeAbsoluteSD(original_sd, new_sd, &new_sd_size, dacl, &dacl_size,
                        sacl, &sacl_size, owner, &owner_size,
                        group, &group_size)) {
    ASSERT_STRING("rlz_lib::CreateMachineState: MakeAbsoluteSD failed");
    return false;
  }

  // If all users already have read/write access to the registry key, then
  // nothing to do.  Otherwise change the security descriptor of the key to
  // give everyone access.
  if (HasAccess(users_sid, KEY_ALL_ACCESS, dacl)) {
    return false;
  }

  // Add ALL-USERS ALL-ACCESS ACL.
  EXPLICIT_ACCESS ea;
  ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
  ea.grfAccessPermissions = GENERIC_ALL | KEY_ALL_ACCESS;
  ea.grfAccessMode = GRANT_ACCESS;
  ea.grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
  ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
  ea.Trustee.ptstrName = L"Everyone";

  ACL* new_dacl = NULL;
  result = SetEntriesInAcl(1, &ea, dacl, &new_dacl);
  if (result != ERROR_SUCCESS) {
    ASSERT_STRING("rlz_lib::CreateMachineState: SetEntriesInAcl failed");
    return false;
  }

  BOOL ok = SetSecurityDescriptorDacl(new_sd, TRUE, new_dacl, FALSE);
  if (!ok) {
    ASSERT_STRING("rlz_lib::CreateMachineState: "
                  "SetSecurityDescriptorOwner failed");
    LocalFree(new_dacl);
    return false;
  }

  result = ::RegSetKeySecurity(hklm_key.Handle(),
                               DACL_SECURITY_INFORMATION,
                               new_sd);
  // Note that the new DACL cannot be freed until after the call to
  // RegSetKeySecurity().
  LocalFree(new_dacl);

  bool success = true;
  if (result != ERROR_SUCCESS) {
    ASSERT_STRING("rlz_lib::CreateMachineState: "
                  "Unable to create / open machine key.");
    success = false;
  }


  return success;
}

bool SetMachineDealCode(const char* dcc) {
  return MachineDealCode::Set(dcc);
}

bool GetMachineDealCodeAsCgi(char* cgi, size_t cgi_size) {
  return MachineDealCode::GetAsCgi(cgi, cgi_size);
}

bool GetMachineDealCode(char* dcc, size_t dcc_size) {
  return MachineDealCode::Get(dcc, dcc_size);
}

// Combined functions.

bool SetMachineDealCodeFromPingResponse(const char* response) {
  return MachineDealCode::SetFromPingResponse(response);
}

void ClearProductState(Product product, const AccessPoint* access_points) {
  LibMutex lock;
  if (lock.failed())
    return;

  UserKey user_key;
  if (!user_key.HasAccess(true))
    return;

  // Delete all product specific state.
  VERIFY(ClearAllProductEvents(product));
  VERIFY(FinancialPing::ClearLastPingTime(product));

  // Delete all RLZ's for access points being uninstalled.
  if (access_points) {
    for (int i = 0; access_points[i] != NO_ACCESS_POINT; i++) {
      VERIFY(SetAccessPointRlz(access_points[i], ""));
    }
  }

  // Delete each of the known subkeys if empty.
  const wchar_t* subkeys[] = {
    kRlzsSubkeyName,
    kEventsSubkeyName,
    kStatefulEventsSubkeyName,
    kPingTimesSubkeyName
  };

  for (int i = 0; i < arraysize(subkeys); i++) {
    std::wstring subkey_name;
    base::StringAppendF(&subkey_name, L"%ls\\%ls", kLibKeyName, subkeys[i]);
    AppendBrandToString(&subkey_name);

    VERIFY(DeleteKeyIfEmpty(user_key.Get(), subkey_name.c_str()));
  }

  // Delete the library key and its parents too now if empty.
  VERIFY(DeleteKeyIfEmpty(user_key.Get(), kLibKeyName));
  VERIFY(DeleteKeyIfEmpty(user_key.Get(), kGoogleCommonKeyName));
  VERIFY(DeleteKeyIfEmpty(user_key.Get(), kGoogleKeyName));
}

bool GetMachineId(wchar_t* buffer, size_t buffer_size) {
  if (!buffer || buffer_size <= kMachineIdLength)
    return false;
  buffer[0] = 0;

  std::wstring machine_id;
  if (!MachineDealCode::GetMachineId(&machine_id))
    return false;

  wcsncpy(buffer, machine_id.c_str(), buffer_size);
  buffer[buffer_size - 1] = 0;
  return true;
}

void InitializeTempHivesForTesting(const base::win::RegKey& temp_hklm_key,
                                   const base::win::RegKey& temp_hkcu_key) {
  // For the moment, the HKCU hive requires no initialization.

  if (base::win::GetVersion() >= base::win::VERSION_WIN7) {
    // Copy the following HKLM subtrees to the temporary location so that the
    // win32 APIs used by the tests continue to work:
    //
    //    HKLM\System\CurrentControlSet\Control\Lsa\AccessProviders
    //
    // This seems to be required since Win7.
    base::win::RegKey dest(temp_hklm_key.Handle(), kHKLMAccessProviders,
                           KEY_ALL_ACCESS);
    CopyRegistryTree(base::win::RegKey(HKEY_LOCAL_MACHINE,
                                       kHKLMAccessProviders,
                                       KEY_READ),
                     &dest);
  }
}

}  // namespace rlz_lib
