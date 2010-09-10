// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Library functions related to the OEM Deal Confirmation Code.

#include "rlz/win/lib/machine_deal.h"

#include <windows.h>
#include <Sddl.h>  // For ConvertSidToStringSidW.
#include <vector>

#include "base/basictypes.h"
#include "base/registry.h"
#include "base/scoped_ptr.h"
#include "base/sha1.h"
#include "base/string_util.h"
#include "rlz/win/lib/assert.h"
#include "rlz/win/lib/crc8.h"
#include "rlz/win/lib/lib_mutex.h"
#include "rlz/win/lib/lib_values.h"
#include "rlz/win/lib/process_info.h"
#include "rlz/win/lib/string_utils.h"
#include "rlz/win/lib/user_key.h"


namespace {

// Current DCC can only uses [a-zA-Z0-9_-!@$*();.<>,:]
// We will be more liberal and allow some additional chars, but not url meta
// chars.
bool IsGoodDccChar(char ch) {
  if (IsAsciiAlpha(ch) || IsAsciiDigit(ch))
    return true;

  switch (ch) {
    case '_':
    case '-':
    case '!':
    case '@':
    case '$':
    case '*':
    case '(':
    case ')':
    case ';':
    case '.':
    case '<':
    case '>':
    case ',':
    case ':':
      return true;
  }

  return false;
}

// This function will remove bad rlz chars and also limit the max rlz to some
// reasonable size.  It also assumes that normalized_dcc is at least
// kMaxDccLength+1 long.
void NormalizeDcc(const char* raw_dcc, char* normalized_dcc) {
  int index = 0;
  for (; raw_dcc[index] != 0 && index < rlz_lib::kMaxDccLength; ++index) {
    char current = raw_dcc[index];
    if (IsGoodDccChar(current)) {
      normalized_dcc[index] = current;
    } else {
      normalized_dcc[index] = '.';
    }
  }

  normalized_dcc[index] = 0;
}

bool GetResponseLine(const char* response_text, int response_length,
                     int* search_index, std::string* response_line) {
  if (!response_line || !search_index || *search_index > response_length)
    return false;

  response_line->clear();

  if (*search_index < 0)
    return false;

  int line_begin = *search_index;
  const char* line_end = strchr(response_text + line_begin, '\n');

  if (line_end == NULL || line_end - response_text > response_length) {
    line_end = response_text + response_length;
    *search_index = -1;
  } else {
    *search_index = line_end - response_text + 1;
  }

  response_line->assign(response_text + line_begin,
                        line_end - response_text - line_begin);
  return true;
}

bool GetResponseValue(const std::string& response_line,
                      const std::string& response_key,
                      std::string* value) {
  if (!value)
    return false;

  value->clear();

  if (!StartsWithASCII(response_line, response_key, true))
    return false;

  std::vector<std::string> tokens;
  SplitString(response_line, ':', &tokens);
  if (tokens.size() != 2)
    return false;

  // The first token is the key, the second is the value.  The value is already
  // trimmed for whitespace.
  *value = tokens[1];
  return true;
}

}  // namespace anonymous


namespace rlz_lib {

bool MachineDealCode::Set(const char* dcc) {
  LibMutex lock;
  if (lock.failed())
    return false;

  // TODO: if (!ProcessInfo::CanWriteMachineKey()) return false;

  // Validate the new dcc value.
  size_t length = strlen(dcc);
  if (length >  kMaxDccLength) {
    ASSERT_STRING("MachineDealCode::Set: DCC length is exceeds max allowed.");
    return false;
  }

  RegKey hklm_key(HKEY_LOCAL_MACHINE, kLibKeyName,
                  KEY_READ | KEY_WRITE | KEY_WOW64_32KEY);
  if (!hklm_key.Valid()) {
    ASSERT_STRING("MachineDealCode::Set: Unable to create / open machine key."
                  " Did you call rlz_lib::CreateMachineState()?");
    return false;
  }

  char normalized_dcc[kMaxDccLength + 1];
  NormalizeDcc(dcc, normalized_dcc);
  VERIFY(length == strlen(normalized_dcc));

  // Write the DCC to HKLM.  Note that we need to include the null character
  // when writing the string.
  if (!RegKeyWriteValue(hklm_key, kDccValueName, normalized_dcc)) {
    ASSERT_STRING("MachineDealCode::Set: Could not write the DCC value");
    return false;
  }

  return true;
}


bool MachineDealCode::GetNewCodeFromPingResponse(const char* response,
    bool* has_new_dcc, char* new_dcc, int new_dcc_size) {
  if (!has_new_dcc || !new_dcc || !new_dcc_size)
    return false;

  *has_new_dcc = false;
  new_dcc[0] = 0;

  int response_length = -1;
  if (!IsPingResponseValid(response, &response_length))
    return false;

  // Get the current DCC value to compare to later)
  char stored_dcc[kMaxDccLength + 1];
  if (!Get(stored_dcc, arraysize(stored_dcc)))
    stored_dcc[0] = 0;

  int search_index = 0;
  std::string response_line;
  std::string new_dcc_value;
  bool old_dcc_confirmed = false;
  const std::string dcc_cgi(kDccCgiVariable);
  const std::string dcc_cgi_response(kSetDccResponseVariable);
  while (GetResponseLine(response, response_length, &search_index,
                         &response_line)) {
    std::string value;

    if (!old_dcc_confirmed &&
        GetResponseValue(response_line, dcc_cgi, &value)) {
      // This is the old DCC confirmation - should match value in registry.
      if (value != stored_dcc)
        return false;  // Corrupted DCC - ignore this response.
      else
        old_dcc_confirmed = true;
      continue;
    }

    if (!(*has_new_dcc) &&
        GetResponseValue(response_line, dcc_cgi_response, &value)) {
      // This is the new DCC.
      if (value.size() > kMaxDccLength) continue;  // Too long
      *has_new_dcc = true;
      new_dcc_value = value;
    }
  }

  old_dcc_confirmed |= (NULL == stored_dcc[0]);

  base::strlcpy(new_dcc, new_dcc_value.c_str(), new_dcc_size);
  return old_dcc_confirmed;
}

bool MachineDealCode::SetFromPingResponse(const char* response) {
  bool has_new_dcc = false;
  char new_dcc[kMaxDccLength + 1];

  bool response_valid = GetNewCodeFromPingResponse(
      response, &has_new_dcc, new_dcc, arraysize(new_dcc));

  if (response_valid && has_new_dcc)
    return Set(new_dcc);

  return response_valid;
}


bool MachineDealCode::GetAsCgi(char* cgi, int cgi_size) {
  if (!cgi || cgi_size <= 0) {
    ASSERT_STRING("MachineDealCode::GetAsCgi: Invalid buffer");
    return false;
  }

  cgi[0] = 0;

  std::string cgi_arg;
  StringAppendF(&cgi_arg, "%s=", kDccCgiVariable);
  int cgi_arg_length = cgi_arg.size();

  if (cgi_arg_length >= cgi_size) {
    ASSERT_STRING("MachineDealCode::GetAsCgi: Insufficient buffer size");
    return false;
  }

  base::strlcpy(cgi, cgi_arg.c_str(), cgi_size);

  if (!Get(cgi + cgi_arg_length, cgi_size - cgi_arg_length)) {
    cgi[0] = 0;
    return false;
  }
  return true;
}


bool MachineDealCode::Get(char* dcc, int dcc_size) {
  LibMutex lock;
  if (lock.failed())
    return false;

  if (!dcc || dcc_size <= 0) {
    ASSERT_STRING("MachineDealCode::Get: Invalid buffer");
    return false;
  }

  dcc[0] = 0;

  RegKey dcc_key(HKEY_LOCAL_MACHINE, kLibKeyName, KEY_READ | KEY_WOW64_32KEY);
  if (!dcc_key.Valid())
    return false;  // no DCC key.

  size_t size = dcc_size;
  if (!RegKeyReadValue(dcc_key, kDccValueName, dcc, &size)) {
    ASSERT_STRING("MachineDealCode::Get: Insufficient buffer size");
    dcc[0] = 0;
    return false;
  }

  return true;
}


bool MachineDealCode::Clear() {
  RegKey dcc_key(HKEY_LOCAL_MACHINE, kLibKeyName,
                 KEY_READ | KEY_WRITE | KEY_WOW64_32KEY);
  if (!dcc_key.Valid())
    return false;  // no DCC key.

  dcc_key.DeleteValue(kDccValueName);

  // Verify deletion.
  wchar_t dcc[kMaxDccLength + 1];
  DWORD dcc_size = arraysize(dcc);
  if (dcc_key.ReadValue(kDccValueName, dcc, &dcc_size)) {
    ASSERT_STRING("MachineDealCode::Clear: Could not delete the DCC value.");
    return false;
  }

  return true;
}

static bool GetSystemVolumeSerialNumber(int* number) {
  if (!number)
    return false;

  *number = 0;

  // Find the system root path (e.g: C:\).
  wchar_t system_path[MAX_PATH + 1];
  if (!GetSystemDirectoryW(system_path, MAX_PATH))
    return false;

  wchar_t* first_slash = wcspbrk(system_path, L"\\/");
  if (first_slash != NULL)
    *(first_slash + 1) = 0;

  DWORD number_local = 0;
  if (!GetVolumeInformationW(system_path, NULL, 0, &number_local, NULL, NULL,
                             NULL, 0))
    return false;

  *number = number_local;
  return true;
}

bool GetComputerSid(const wchar_t* account_name, SID* sid, DWORD sid_size) {
  static const DWORD kStartDomainLength = 128;  // reasonable to start with

  scoped_array<wchar_t> domain_buffer(new wchar_t[kStartDomainLength]);
  DWORD domain_size = kStartDomainLength;
  DWORD sid_dword_size = sid_size;
  SID_NAME_USE sid_name_use;

  BOOL success = ::LookupAccountNameW(NULL, account_name, sid,
                                      &sid_dword_size, domain_buffer.get(),
                                      &domain_size, &sid_name_use);
  if (!success && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    // We could have gotten the insufficient buffer error because
    // one or both of sid and szDomain was too small. Check for that
    // here.
    if (sid_dword_size > sid_size)
      return false;

    if (domain_size > kStartDomainLength)
      domain_buffer.reset(new wchar_t[domain_size]);

    success = ::LookupAccountNameW(NULL, account_name, sid, &sid_dword_size,
                                   domain_buffer.get(), &domain_size,
                                   &sid_name_use);
  }

  return success;
}

std::wstring ConvertSidToString(SID* sid) {
  std::wstring sid_string;
#if _WIN32_WINNT >= 0x500
  wchar_t* sid_buffer = NULL;
  if (ConvertSidToStringSidW(sid, &sid_buffer)) {
    sid_string = sid_buffer;
    LocalFree(sid_buffer);
  }
#else
  SID_IDENTIFIER_AUTHORITY* sia = ::GetSidIdentifierAuthority(sid);

  if(sia->Value[0] || sia->Value[1]) {
    SStringPrintf(&sid_string, L"S-%d-0x%02hx%02hx%02hx%02hx%02hx%02hx",
        SID_REVISION, (USHORT)sia->Value[0], (USHORT)sia->Value[1],
        (USHORT)sia->Value[2], (USHORT)sia->Value[3], (USHORT)sia->Value[4],
        (USHORT)sia->Value[5]);
  } else {
    ULONG authority = 0;
    for (int i = 2; i < 6; ++i) {
      authority <<= 8;
      authority |= sia->Value[i];
    }

    SStringPrintf(&sid_string, L"S-%d-%lu", SID_REVISION, authority);
  }

  int sub_auth_count = *::GetSidSubAuthorityCount(sid);
  for(int i = 0; i < sub_auth_count; ++i)
    StringAppendF(&sid_string, L"-%lu", *::GetSidSubAuthority(sid, i));
#endif

  return sid_string;
}

bool MachineDealCode::GetMachineId(std::wstring* machine_id) {
  if (!machine_id)
    return false;

  machine_id->clear();

  static std::wstring calculated_id;
  static bool calculated = false;
  if (calculated) {
    *machine_id = calculated_id;
    return true;
  }

  // The ID should be the SID hash + the Hard Drive SNo. + checksum byte.
  static const int kSizeWithoutChecksum = base::SHA1_LENGTH + sizeof(int);
  std::basic_string<unsigned char> id_binary(kSizeWithoutChecksum + 1, 0);

  // Calculate the Windows SID.
  std::wstring sid_string;
  wchar_t computer_name[MAX_COMPUTERNAME_LENGTH + 1] = {0};
  DWORD size = arraysize(computer_name);

  if (GetComputerNameW(computer_name, &size)) {
    char sid_buffer[SECURITY_MAX_SID_SIZE];
    SID* sid = reinterpret_cast<SID*>(sid_buffer);
    if (GetComputerSid(computer_name, sid, SECURITY_MAX_SID_SIZE)) {
      sid_string = ConvertSidToString(sid);
    }
  }

  // Hash the SID.
  if (!sid_string.empty()) {
    // In order to be compatible with the old version of RLZ, the hash of the
    // SID must be done with all the original bytes from the unicode string.
    // However, the chromebase SHA1 hash function takes only an std::string as
    // input, so the unicode string needs to be converted to std::string
    // "as is".
    size_t byte_count = sid_string.size() * sizeof(std::wstring::value_type);
    const char* buffer = reinterpret_cast<const char*>(sid_string.c_str());
    std::string sid_string_buffer(buffer, byte_count);

    // Note that digest can have embedded nulls.
    std::string digest(base::SHA1HashString(sid_string_buffer));
    VERIFY(digest.size() == base::SHA1_LENGTH);
    std::copy(digest.begin(), digest.end(), id_binary.begin());
  }

  // Get the system drive volume serial number.
  int volume_id = 0;
  if (GetSystemVolumeSerialNumber(&volume_id)) {
    // Convert from int to binary (makes big-endian).
    for (int i = 0; i < sizeof(int); i++) {
      int shift_bits = 8 * (sizeof(int) - i - 1);
      id_binary[base::SHA1_LENGTH + i] = static_cast<BYTE>(
          (volume_id >> shift_bits) & 0xFF);
    }
  } else {
    ASSERT_STRING("GetMachineId: Failed to retrieve volume serial number");
    volume_id = 0;
  }

  // Append the checksum byte.
  if (!sid_string.empty() || (0 != volume_id))
    Crc8::Generate(id_binary.c_str(),
                   kSizeWithoutChecksum, &id_binary[kSizeWithoutChecksum]);

  if (!BytesToString(id_binary.c_str(), kSizeWithoutChecksum + 1,
                     &calculated_id))
    return false;

  calculated = true;
  *machine_id = calculated_id;
  return true;
}

};  // namespace
