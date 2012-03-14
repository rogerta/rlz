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

// Event information returned from ping response.
struct ReturnedEvent {
  rlz_lib::AccessPoint access_point;
  rlz_lib::Event event_type;
};

// Helper functions

bool IsAccessPointSupported(rlz_lib::AccessPoint point) {
  switch (point) {
  case rlz_lib::NO_ACCESS_POINT:
  case rlz_lib::LAST_ACCESS_POINT:

  case rlz_lib::MOBILE_IDLE_SCREEN_BLACKBERRY:
  case rlz_lib::MOBILE_IDLE_SCREEN_WINMOB:
  case rlz_lib::MOBILE_IDLE_SCREEN_SYMBIAN:
    // These AP's are never available on Windows PCs.
    return false;

  case rlz_lib::IE_DEFAULT_SEARCH:
  case rlz_lib::IE_HOME_PAGE:
  case rlz_lib::IETB_SEARCH_BOX:
  case rlz_lib::QUICK_SEARCH_BOX:
  case rlz_lib::GD_DESKBAND:
  case rlz_lib::GD_SEARCH_GADGET:
  case rlz_lib::GD_WEB_SERVER:
  case rlz_lib::GD_OUTLOOK:
  case rlz_lib::CHROME_OMNIBOX:
  case rlz_lib::CHROME_HOME_PAGE:
    // TODO: Figure out when these settings are set to Google.

  default:
    return true;
  }
}

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

void GetEventsFromResponseString(
    const std::string& response_line,
    const std::string& field_header,
    std::vector<ReturnedEvent>* event_array) {
  // Get the string of events.
  std::string events = response_line.substr(field_header.size());
  TrimWhitespaceASCII(events, TRIM_LEADING, &events);

  int events_length = events.find_first_of("\r\n ");
  if (events_length < 0)
    events_length = events.size();
  events = events.substr(0, events_length);

  // Break this up into individual events
  int event_end_index = -1;
  do {
    int event_begin = event_end_index + 1;
    event_end_index = events.find(rlz_lib::kEventsCgiSeparator, event_begin);
    int event_end = event_end_index;
    if (event_end < 0)
      event_end = events_length;

    std::string event_string = events.substr(event_begin,
                                             event_end - event_begin);
    if (event_string.size() != 3)  // 3 = 2(AP) + 1(E)
      continue;

    rlz_lib::AccessPoint point = rlz_lib::NO_ACCESS_POINT;
    rlz_lib::Event event = rlz_lib::INVALID_EVENT;
    if (!GetAccessPointFromName(event_string.substr(0, 2).c_str(), &point) ||
        point == rlz_lib::NO_ACCESS_POINT) {
      continue;
    }

    if (!GetEventFromName(event_string.substr(event_string.size() - 1).c_str(),
                          &event) || event == rlz_lib::INVALID_EVENT) {
      continue;
    }

    ReturnedEvent current_event = {point, event};
    event_array->push_back(current_event);
  } while (event_end_index >= 0);
}

// Event storage functions.
bool RecordStatefulEvent(rlz_lib::Product product, rlz_lib::AccessPoint point,
                         rlz_lib::Event event) {
  rlz_lib::ScopedRlzValueStoreLock lock;
  rlz_lib::RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(rlz_lib::RlzValueStore::kWriteAccess))
    return false;

  // Write the new event to the value store.
  const char* point_name = GetAccessPointName(point);
  const char* event_name = GetEventName(event);
  if (!point_name || !event_name)
    return false;

  if (!point_name[0] || !event_name[0])
    return false;

  std::string new_event_value;
  base::StringAppendF(&new_event_value, "%s%s", point_name, event_name);
  return store->AddStatefulEvent(product, new_event_value.c_str());
}

LONG GetProductEventsAsCgiHelper(rlz_lib::Product product, char* cgi,
                                 size_t cgi_size) {
  // Prepend the CGI param key to the buffer.
  std::string cgi_arg;
  base::StringAppendF(&cgi_arg, "%s=", rlz_lib::kEventsCgiVariable);
  if (cgi_size <= cgi_arg.size())
    return ERROR_MORE_DATA;

  size_t index;
  for (index = 0; index < cgi_arg.size(); ++index)
    cgi[index] = cgi_arg[index];

  // Open the events key.
  base::win::RegKey events;
  GetEventsRegKey(rlz_lib::kEventsSubkeyName, &product, KEY_READ,
                  &events);
  if (!events.Valid())
    return ERROR_PATH_NOT_FOUND;

  // Append the events to the buffer.
  size_t buffer_size = cgi_size - cgi_arg.size();
  int num_values = 0;
  LONG result = ERROR_SUCCESS;

  for (num_values = 0; result == ERROR_SUCCESS; ++num_values) {
    cgi[index] = '\0';

    int divider = num_values > 0 ? 1 : 0;
    DWORD size = cgi_size - (index + divider);
    if (size <= 0)
      return ERROR_MORE_DATA;

    result = RegEnumValueA(events.Handle(), num_values, cgi + index + divider,
                           &size, NULL, NULL, NULL, NULL);
    if (result == ERROR_SUCCESS) {
      if (divider)
        cgi[index] = rlz_lib::kEventsCgiSeparator;

      index += size + divider;
    }
  }

  num_values--;
  cgi[index] = '\0';

  if (result == ERROR_MORE_DATA)
    return result;

  return (result == ERROR_NO_MORE_ITEMS && num_values > 0) ?
    ERROR_SUCCESS : ERROR_FILE_NOT_FOUND;
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

bool RecordProductEvent(Product product, AccessPoint point, Event event) {
  ScopedRlzValueStoreLock lock;
  RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(RlzValueStore::kWriteAccess))
    return false;

  const wchar_t* product_name = GetProductName(product);
  if (!product_name)
    return false;

  // Get this event's value.
  const char* point_name = GetAccessPointName(point);
  const char* event_name = GetEventName(event);
  if (!point_name || !event_name)
    return false;

  if (!point_name[0] || !event_name[0])
    return false;

  std::string new_event_value;
  base::StringAppendF(&new_event_value, "%s%s", point_name, event_name);

  // Check whether this event is a stateful event. If so, don't record it.
  if (store->IsStatefulEvent(product, new_event_value.c_str())) {
    // For a stateful event we skip recording, this function is also
    // considered successful.
    return true;
  }

  // Write the new event to registry.
  std::wstring new_event_value_wide(ASCIIToWide(new_event_value));
  DWORD value = 1;
  base::win::RegKey reg_key;
  rlz_lib::GetEventsRegKey(kEventsSubkeyName, &product,
                           KEY_WRITE, &reg_key);
  if (reg_key.WriteValue(
      new_event_value_wide.c_str(), value) != ERROR_SUCCESS) {
    ASSERT_STRING("RecordProductEvent: Could not write the new event value");
    return false;
  }

  return true;
}

bool ClearProductEvent(Product product, AccessPoint point, Event event) {
  LibMutex lock;
  if (lock.failed())
    return false;

  UserKey user_key;
  if (!user_key.HasAccess(true))
    return false;

  // Get the event's registry value and delete it.
  const char* point_name = GetAccessPointName(point);
  const char* event_name = GetEventName(event);
  if (!point_name || !event_name)
    return false;

  if (!point_name[0] || !event_name[0])
    return false;

  std::wstring point_name_wide(ASCIIToWide(point_name));
  std::wstring event_name_wide(ASCIIToWide(event_name));
  std::wstring event_value;
  base::StringAppendF(&event_value, L"%ls%ls", point_name_wide.c_str(),
                      event_name_wide.c_str());
  base::win::RegKey key;
  GetEventsRegKey(kEventsSubkeyName, &product, KEY_WRITE, &key);
  key.DeleteValue(event_value.c_str());

  // Verify deletion.
  DWORD value;
  if (key.ReadValueDW(event_value.c_str(), &value) == ERROR_SUCCESS) {
    ASSERT_STRING("ClearProductEvent: Could not delete the event value.");
    return false;
  }

  return true;
}

bool GetProductEventsAsCgi(Product product, char* cgi, size_t cgi_size) {
  if (!cgi || cgi_size <= 0) {
    ASSERT_STRING("GetProductEventsAsCgi: Invalid buffer");
    return false;
  }

  cgi[0] = 0;

  LibMutex lock;
  if (lock.failed())
    return false;

  UserKey user_key;
  if (!user_key.HasAccess(false))
    return false;

  DWORD size_local =
      cgi_size <= kMaxCgiLength + 1 ? cgi_size : kMaxCgiLength + 1;
  UINT length = 0;
  LONG result = GetProductEventsAsCgiHelper(product, cgi, size_local);
  if (result == ERROR_MORE_DATA && cgi_size >= (kMaxCgiLength + 1))
    result = ERROR_SUCCESS;

  if (result != ERROR_SUCCESS) {
    if (result == ERROR_MORE_DATA)
      ASSERT_STRING("GetProductEventsAsCgi: Insufficient buffer size");
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

// TODO: Use something like RSA to make sure the response is
// from a Google server.
bool ParsePingResponse(Product product, const char* response) {
  LibMutex lock;
  if (lock.failed())
    return false;

  UserKey user_key;
  if (!user_key.HasAccess(true))
    return false;

  std::string response_string(response);
  int response_length = -1;
  if (!IsPingResponseValid(response, &response_length))
    return false;

  if (0 == response_length)
    return true;  // Empty response - no parsing.

  std::string events_variable;
  std::string stateful_events_variable;
  base::SStringPrintf(&events_variable, "%s: ", kEventsCgiVariable);
  base::SStringPrintf(&stateful_events_variable, "%s: ",
                      kStatefulEventsCgiVariable);

  int rlz_cgi_length = strlen(kRlzCgiVariable);

  // Split response lines. Expected response format is lines of the form:
  // rlzW1: 1R1_____en__252
  int line_end_index = -1;
  do {
    int line_begin = line_end_index + 1;
    line_end_index = response_string.find("\n", line_begin);

    int line_end = line_end_index;
    if (line_end < 0)
      line_end = response_length;

    if (line_end <= line_begin)
      continue;  // Empty line.

    std::string response_line;
    response_line = response_string.substr(line_begin, line_end - line_begin);

    if (StartsWithASCII(response_line, kRlzCgiVariable, true)) {  // An RLZ.
      int separator_index = -1;
      if ((separator_index = response_line.find(": ")) < 0)
        continue;  // Not a valid key-value pair.

      // Get the access point.
      std::string point_name =
        response_line.substr(3, separator_index - rlz_cgi_length);
      AccessPoint point = NO_ACCESS_POINT;
      if (!GetAccessPointFromName(point_name.c_str(), &point) ||
          point == NO_ACCESS_POINT)
        continue;  // Not a valid access point.

      // Get the new RLZ.
      std::string rlz_value(response_line.substr(separator_index + 2));
      TrimWhitespaceASCII(rlz_value, TRIM_LEADING, &rlz_value);

      int rlz_length = rlz_value.find_first_of("\r\n ");
      if (rlz_length < 0)
        rlz_length = rlz_value.size();

      if (rlz_length > kMaxRlzLength)
        continue;  // Too long.

      if (IsAccessPointSupported(point))
        SetAccessPointRlz(point, rlz_value.substr(0, rlz_length).c_str());
    } else if (StartsWithASCII(response_line, events_variable, true)) {
      // Clear events which server parsed.
      std::vector<ReturnedEvent> event_array;
      GetEventsFromResponseString(response_line, events_variable, &event_array);
      for (size_t i = 0; i < event_array.size(); ++i) {
        ClearProductEvent(product, event_array[i].access_point,
                          event_array[i].event_type);
      }
    } else if (StartsWithASCII(response_line, stateful_events_variable, true)) {
      // Record any stateful events the server send over.
      std::vector<ReturnedEvent> event_array;
      GetEventsFromResponseString(response_line, stateful_events_variable,
                                  &event_array);
      for (size_t i = 0; i < event_array.size(); ++i) {
        RecordStatefulEvent(product, event_array[i].access_point,
                            event_array[i].event_type);
      }
    }
  } while (line_end_index >= 0);

  // Update the DCC in registry if needed.
  MachineDealCode::SetFromPingResponse(response);

  return true;
}

bool SetMachineDealCodeFromPingResponse(const char* response) {
  return MachineDealCode::SetFromPingResponse(response);
}

bool ParseFinancialPingResponse(Product product, const char* response) {
  // Update the last ping time irrespective of success.
  FinancialPing::UpdateLastPingTime(product);
  // Parse the ping response - update RLZs, clear events.
  return ParsePingResponse(product, response);
}

bool SendFinancialPing(Product product, const AccessPoint* access_points,
                       const char* product_signature,
                       const char* product_brand,
                       const char* product_id, const char* product_lang,
                       bool exclude_machine_id) {
  return SendFinancialPing(product, access_points, product_signature,
                           product_brand, product_id, product_lang,
                           exclude_machine_id, false);
}


bool SendFinancialPing(Product product, const AccessPoint* access_points,
                       const char* product_signature,
                       const char* product_brand,
                       const char* product_id, const char* product_lang,
                       bool exclude_machine_id,
                       const bool skip_time_check) {
  // Create the financial ping request.
  std::string request;
  if (!FinancialPing::FormRequest(product, access_points, product_signature,
                                  product_brand, product_id, product_lang,
                                  exclude_machine_id, &request))
    return false;

  // Check if the time is right to ping.
  if (!FinancialPing::IsPingTime(product, skip_time_check))
    return false;

  // Send out the ping, update the last ping time irrespective of success.
  FinancialPing::UpdateLastPingTime(product);
  std::string response;
  if (!FinancialPing::PingServer(request.c_str(), &response))
    return false;

  // Parse the ping response - update RLZs, clear events.
  return ParsePingResponse(product, response.c_str());
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
    SupplementaryBranding::AppendBrandToString(&subkey_name);

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
