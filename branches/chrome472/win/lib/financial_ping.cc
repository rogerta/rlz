// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Library functions related to the Financial Server ping.

#include "rlz/win/lib/financial_ping.h"

#include <windows.h>
#include <wininet.h>
#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "rlz/win/lib/assert.h"
#include "rlz/win/lib/lib_mutex.h"
#include "rlz/win/lib/lib_values.h"
#include "rlz/win/lib/machine_deal.h"
#include "rlz/win/lib/string_utils.h"
#include "rlz/win/lib/user_key.h"


namespace {

int64 ToInt64(const FILETIME& ft) {
  LARGE_INTEGER integer;
  integer.HighPart = ft.dwHighDateTime;
  integer.LowPart = ft.dwLowDateTime;
  return integer.QuadPart;
}

int64 GetSystemTimeAsInt64() {
  FILETIME now_as_file_time;
  GetSystemTimeAsFileTime(&now_as_file_time);
  return ToInt64(now_as_file_time);
}

}  // namespace anonymous


namespace rlz_lib {

class InternetHandle {
 public:
  InternetHandle(HINTERNET handle) { handle_ = handle; }
  ~InternetHandle() { if (handle_) InternetCloseHandle(handle_); }
  operator HINTERNET() const { return handle_; }
  bool operator!() const { return (handle_ == NULL); }

 private:
  HINTERNET handle_;
};

bool FinancialPing::FormRequest(Product product,
    const AccessPoint* access_points, const char* product_signature,
    const char* product_brand, const char* product_id,
    const char* product_lang, bool exclude_machine_id, const wchar_t* sid,
    std::string* request) {
  if (!request) {
    ASSERT_STRING("FinancialPing::FormRequest: request is NULL");
    return false;
  }

  request->clear();

  LibMutex lock;
  if (lock.failed())
    return false;

  UserKey user_key(sid);
  if (!user_key.HasAccess(false))
    return false;

  if (!access_points) {
    ASSERT_STRING("FinancialPing::FormRequest: access_points is NULL");
    return false;
  }

  if (!product_signature) {
    ASSERT_STRING("FinancialPing::FormRequest: product_signature is NULL");
    return false;
  }

  StringAppendF(request, "%s?", kFinancialPingPath);

  // Add the signature, brand, product id and language.
  StringAppendF(request, "%s=%s", kProductSignatureCgiVariable,
                product_signature);
  if (product_brand)
    StringAppendF(request, "&%s=%s", kProductBrandCgiVariable,
                  product_brand);

  if (product_id)
    StringAppendF(request, "&%s=%s", kProductIdCgiVariable, product_id);

  if (product_lang)
    StringAppendF(request, "&%s=%s", kProductLanguageCgiVariable,
                  product_lang);

  // Add the product events.
  char cgi[kMaxCgiLength + 1];
  cgi[0] = 0;
  bool has_events = GetProductEventsAsCgi(product, cgi, arraysize(cgi), sid);
  if (has_events)
    StringAppendF(request, "&%s", cgi);

  // If we don't have any events, we should ping all the AP's on the system
  // that we know about and have a current RLZ value, even if they are not
  // used by this product.
  AccessPoint all_points[LAST_ACCESS_POINT];
  if (!has_events) {
    char rlz[kMaxRlzLength + 1];
    int idx = 0;
    for (int ap = NO_ACCESS_POINT + 1; ap < LAST_ACCESS_POINT; ap++) {
      rlz[0] = 0;
      AccessPoint point = static_cast<AccessPoint>(ap);
      if (GetAccessPointRlz(point, rlz, arraysize(rlz), sid) &&
          rlz[0] != NULL)
        all_points[idx++] = point;
    }
    all_points[idx] = NO_ACCESS_POINT;
  }

  // Add the RLZ's and the DCC if needed. This is the same as get PingParams.
  // This will also include the RLZ Exchange Protocol CGI Argument.
  cgi[0] = 0;
  if (GetPingParams(product, has_events ? access_points : all_points,
                    cgi, arraysize(cgi), sid))
    StringAppendF(request, "&%s", cgi);

  if (has_events && !exclude_machine_id) {
    std::wstring machine_id;
    if (MachineDealCode::GetMachineId(&machine_id)) {
      StringAppendF(request, "&%s=%ls", kMachineIdCgiVariable,
                    machine_id.c_str());
    }
  }

  return true;
}

bool FinancialPing::PingServer(const char* request, std::string* response) {
  if (!response)
    return false;

  response->clear();

  // Initialize WinInet.
  InternetHandle inet_handle = InternetOpenA(kFinancialPingUserAgent,
                                             INTERNET_OPEN_TYPE_PRECONFIG,
                                             NULL, NULL, 0);
  if (!inet_handle)
    return false;

  // Open network connection.
  InternetHandle connection_handle = InternetConnectA(inet_handle,
      kFinancialServer, kFinancialPort, "", "", INTERNET_SERVICE_HTTP,
      INTERNET_FLAG_NO_CACHE_WRITE, 0);
  if (!connection_handle)
    return false;

  // Prepare the HTTP request.
  InternetHandle http_handle = HttpOpenRequestA(connection_handle,
      kFinancialPingType, request, NULL, NULL, kFinancialPingResponseObjects,
      INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_COOKIES, NULL);
  if (!http_handle)
    return false;

  // Send the HTTP request. Note: Fails if user is working in off-line mode.
  if (!HttpSendRequest(http_handle, NULL, 0, NULL, 0))
    return false;

  // Check the response status.
  DWORD status;
  DWORD status_size = sizeof(status);
  if (!HttpQueryInfo(http_handle, HTTP_QUERY_STATUS_CODE |
                     HTTP_QUERY_FLAG_NUMBER, &status, &status_size, NULL) ||
      200 != status)
    return false;

  // Get the response text.
  scoped_array<char> buffer(new char[kMaxPingResponseLength]);
  if (buffer.get() == NULL)
    return false;

  DWORD bytes_read = 0;
  while (InternetReadFile(http_handle, buffer.get(), kMaxPingResponseLength,
                          &bytes_read) && bytes_read > 0) {
    response->append(buffer.get(), bytes_read);
    bytes_read = 0;
  };

  return true;
}


bool FinancialPing::ParseResponse(Product product, const char* response,
                                  const wchar_t* sid) {
  return ParsePingResponse(product, response, sid);
}

bool FinancialPing::IsPingTime(Product product, const wchar_t* sid,
                               bool no_delay) {
  LibMutex lock;
  if (lock.failed())
    return false;

  UserKey user_key(sid);
  if (!user_key.HasAccess(false))
    return false;

  std::wstring key_location;
  StringAppendF(&key_location, L"%ls\\%ls", kLibKeyName, kPingTimesSubkeyName);

  uint64 last_ping;
  DWORD size;
  DWORD type;
  RegKey key(user_key.Get(), key_location.c_str(), KEY_READ);
  if (!key.ReadValue(GetProductName(product), &last_ping, &size, &type))
    return true;

  VERIFY(REG_QWORD == type);
  VERIFY(sizeof(last_ping) == size);

  uint64 now = GetSystemTimeAsInt64();
  int64 interval = now - last_ping;

  // If interval is negative, clock was probably reset. So ping.
  if (interval < 0)
    return true;

  // Check if this product has any unreported events.
  char cgi[kMaxCgiLength + 1];
  cgi[0] = 0;
  bool has_events = GetProductEventsAsCgi(product, cgi, arraysize(cgi), sid);
  if (no_delay && has_events)
    return true;

  return interval >= (has_events ? kEventsPingInterval : kNoEventsPingInterval);
}


bool FinancialPing::UpdateLastPingTime(Product product, const wchar_t* sid) {
  LibMutex lock;
  if (lock.failed())
    return false;

  UserKey user_key(sid);
  if (!user_key.HasAccess(true))
    return false;

  uint64 now = GetSystemTimeAsInt64();

  std::wstring key_location;
  StringAppendF(&key_location, L"%ls\\%ls", kLibKeyName, kPingTimesSubkeyName);

  RegKey key(user_key.Get(), key_location.c_str(), KEY_WRITE);
  return key.WriteValue(GetProductName(product), &now, sizeof(now), REG_QWORD);
}


bool FinancialPing::ClearLastPingTime(Product product, const wchar_t* sid) {
  LibMutex lock;
  if (lock.failed())
    return false;

  UserKey user_key(sid);
  if (!user_key.HasAccess(true))
    return false;

  std::wstring key_location;
  StringAppendF(&key_location, L"%ls\\%ls", kLibKeyName, kPingTimesSubkeyName);

  const wchar_t* value_name = GetProductName(product);
  RegKey key(user_key.Get(), key_location.c_str(), KEY_WRITE);
  key.DeleteValue(value_name);

  // Verify deletion.
  uint64 value;
  DWORD size = sizeof(value);
  if (key.ReadValue(value_name, &value, &size)) {
    ASSERT_STRING("FinancialPing::ClearLastPingTime: Failed to delete value.");
    return false;
  }

  return true;
}

}  // namespace
