// Copyright 2012 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A library to manage RLZ information for access-points shared
// across different client applications.

#include "rlz/lib/rlz_lib.h"

#include "base/string_util.h"
#include "base/stringprintf.h"
#include "rlz/lib/assert.h"
#include "rlz/lib/crc32.h"
#include "rlz/lib/financial_ping.h"
#include "rlz/lib/lib_values.h"
#include "rlz/lib/rlz_value_store.h"
#include "rlz/lib/string_utils.h"

namespace {

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

// Current RLZ can only use [a-zA-Z0-9_\-]
// We will be more liberal and allow some additional chars, but not url meta
// chars.
bool IsGoodRlzChar(const char ch) {
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
    return true;
  }

  return false;
}

// This function will remove bad rlz chars and also limit the max rlz to some
// reasonable size.  It also assumes that normalized_rlz is at least
// kMaxRlzLength+1 long.
void NormalizeRlz(const char* raw_rlz, char* normalized_rlz) {
  int index = 0;
  for (; raw_rlz[index] != 0 && index < rlz_lib::kMaxRlzLength; ++index) {
    char current = raw_rlz[index];
    if (IsGoodRlzChar(current)) {
      normalized_rlz[index] = current;
    } else {
      normalized_rlz[index] = '.';
    }
  }

  normalized_rlz[index] = 0;
}

}  // namespace

namespace rlz_lib {

// RLZ storage functions.

bool GetAccessPointRlz(AccessPoint point, char* rlz, size_t rlz_size) {
  if (!rlz || rlz_size <= 0) {
    ASSERT_STRING("GetAccessPointRlz: Invalid buffer");
    return false;
  }

  rlz[0] = 0;

  ScopedRlzValueStoreLock lock;
  RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(RlzValueStore::kReadAccess))
    return false;

  if (!IsAccessPointSupported(point))
    return false;

  return store->ReadAccessPointRlz(point, rlz, rlz_size);
}

bool SetAccessPointRlz(AccessPoint point, const char* new_rlz) {
  ScopedRlzValueStoreLock lock;
  RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(RlzValueStore::kWriteAccess))
    return false;

  if (!new_rlz) {
    ASSERT_STRING("SetAccessPointRlz: Invalid buffer");
    return false;
  }

  // Return false if the access point is not set to Google.
  if (!IsAccessPointSupported(point)) {
    ASSERT_STRING(("SetAccessPointRlz: "
                "Cannot set RLZ for unsupported access point."));
    return false;
  }

  // Verify the RLZ length.
  size_t rlz_length = strlen(new_rlz);
  if (rlz_length > kMaxRlzLength) {
    ASSERT_STRING("SetAccessPointRlz: RLZ length is exceeds max allowed.");
    return false;
  }

  char normalized_rlz[kMaxRlzLength + 1];
  NormalizeRlz(new_rlz, normalized_rlz);
  VERIFY(strlen(new_rlz) == rlz_length);

  // Setting RLZ to empty == clearing.
  if (normalized_rlz[0] == 0)
    return store->ClearAccessPointRlz(point);
  return store->WriteAccessPointRlz(point, normalized_rlz);
}

// Financial Server pinging functions.

bool PingFinancialServer(Product product, const char* request, char* response,
                         size_t response_buffer_size) {
  if (!response || response_buffer_size == 0)
    return false;
  response[0] = 0;

  // Check if the time is right to ping.
  if (!FinancialPing::IsPingTime(product, false)) return false;

  // Send out the ping.
  std::string response_string;
  if (!FinancialPing::PingServer(request, &response_string))
    return false;

  if (response_string.size() >= response_buffer_size)
    return false;

  strncpy(response, response_string.c_str(), response_buffer_size);
  response[response_buffer_size - 1] = 0;
  return true;
}

bool IsPingResponseValid(const char* response, int* checksum_idx) {
  if (!response || !response[0])
    return false;

  if (checksum_idx)
    *checksum_idx = -1;

  if (strlen(response) > kMaxPingResponseLength) {
    ASSERT_STRING("IsPingResponseValid: response is too long to parse.");
    return false;
  }

  // Find the checksum line.
  std::string response_string(response);

  std::string checksum_param("\ncrc32: ");
  int calculated_crc;
  int checksum_index = response_string.find(checksum_param);
  if (checksum_index >= 0) {
    // Calculate checksum of message preceeding checksum line.
    // (+ 1 to include the \n)
    std::string message(response_string.substr(0, checksum_index + 1));
    if (!Crc32(message.c_str(), &calculated_crc))
      return false;
  } else {
    checksum_param = "crc32: ";  // Empty response case.
    if (!StartsWithASCII(response_string, checksum_param, true))
      return false;

    checksum_index = 0;
    if (!Crc32("", &calculated_crc))
      return false;
  }

  // Find the checksum value on the response.
  int checksum_end = response_string.find("\n", checksum_index + 1);
  if (checksum_end < 0)
    checksum_end = response_string.size();

  int checksum_begin = checksum_index + checksum_param.size();
  std::string checksum = response_string.substr(checksum_begin,
      checksum_end - checksum_begin + 1);
  TrimWhitespaceASCII(checksum, TRIM_ALL, &checksum);

  if (checksum_idx)
    *checksum_idx = checksum_index;

  return calculated_crc == HexStringToInteger(checksum.c_str());
}


// Combined functions.

bool GetPingParams(Product product, const AccessPoint* access_points,
                   char* cgi, size_t cgi_size) {
  if (!cgi || cgi_size <= 0) {
    ASSERT_STRING("GetPingParams: Invalid buffer");
    return false;
  }

  cgi[0] = 0;

  // Keep the lock durin gall GetAccessPointRlz() calls below.
  ScopedRlzValueStoreLock lock;
  RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(RlzValueStore::kReadAccess))
    return false;

  if (!access_points) {
    ASSERT_STRING("GetPingParams: access_points is NULL");
    return false;
  }

  // Add the RLZ Exchange Protocol version.
  std::string cgi_string(kProtocolCgiArgument);

  // Copy the &rlz= over.
  base::StringAppendF(&cgi_string, "&%s=", kRlzCgiVariable);

  // Now add each of the RLZ's.
  bool first_rlz = true;  // comma before every RLZ but the first.
  for (int i = 0; access_points[i] != NO_ACCESS_POINT; i++) {
    char rlz[kMaxRlzLength + 1];
    if (GetAccessPointRlz(access_points[i], rlz, arraysize(rlz))) {
      const char* access_point = GetAccessPointName(access_points[i]);
      if (!access_point)
        continue;

      base::StringAppendF(&cgi_string, "%s%s%s%s",
                          first_rlz ? "" : kRlzCgiSeparator,
                          access_point, kRlzCgiIndicator, rlz);
      first_rlz = false;
    }
  }

#if defined(OS_WIN)
  // Report the DCC too if not empty. DCCs are windows-only.
  char dcc[kMaxDccLength + 1];
  dcc[0] = 0;
  if (GetMachineDealCode(dcc, arraysize(dcc)) && dcc[0])
    base::StringAppendF(&cgi_string, "&%s=%s", kDccCgiVariable, dcc);
#endif

  if (cgi_string.size() >= cgi_size)
    return false;

  strncpy(cgi, cgi_string.c_str(), cgi_size);
  cgi[cgi_size - 1] = 0;

  return true;
}

}  // namespace rlz_lib
