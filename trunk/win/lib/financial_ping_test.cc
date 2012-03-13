// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A test application for the FinancialPing class.
//
// These tests should not be executed on the build server:
// - They modify machine state (registry).
//
// These tests require write access to HKCU and HKLM.
//
// The "GGLA" brand is used to test the normal code flow of the code, and the
// "TEST" brand is used to test the supplementary brand code code flow.  In one
// case below, the brand "GOOG" is used because the code wants to use a brand
// that is neither of the two mentioned above.

#include <windows.h>

#include "rlz/lib/financial_ping.h"

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/utf_string_conversions.h"
#include "base/win/registry.h"
#include "rlz/lib/lib_values.h"
#include "rlz/win/lib/machine_deal.h"
#include "rlz/win/test/rlz_test_helpers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

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

// Ping times in 100-nanosecond intervals.
const int64 k1MinuteInterval = 60LL * 10000000LL;  // 1 minute

}  // namespace anonymous

class FinancialPingTest : public RlzLibTestBase {
};

TEST_F(FinancialPingTest, FormRequest) {
  std::wstring brand_wide = rlz_lib::SupplementaryBranding::GetBrand();
  std::string brand_utf8 = WideToUTF8(brand_wide);
  const char* brand = brand_utf8.empty() ? "GGLA" : brand_utf8.c_str();

  EXPECT_TRUE(rlz_lib::MachineDealCode::Set("dcc_value"));

  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX,
      "TbRlzValue"));

  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_HOME_PAGE, rlz_lib::INSTALL));

  rlz_lib::AccessPoint points[] =
    {rlz_lib::IETB_SEARCH_BOX, rlz_lib::NO_ACCESS_POINT,
     rlz_lib::NO_ACCESS_POINT};

  std::wstring machine_id;
  bool got_machine_id = rlz_lib::MachineDealCode::GetMachineId(&machine_id);

  std::string request;
  EXPECT_TRUE(rlz_lib::FinancialPing::FormRequest(rlz_lib::TOOLBAR_NOTIFIER,
      points, "swg", brand, NULL, "en", false, &request));
  std::string expected_response;
  base::StringAppendF(&expected_response,
      "/tools/pso/ping?as=swg&brand=%s&hl=en&"
      "events=I7S,W1I&rep=2&rlz=T4:TbRlzValue&dcc=dcc_value", brand);

  if (got_machine_id)
    base::StringAppendF(&expected_response, "&id=%ls", machine_id.c_str());
  EXPECT_EQ(expected_response, request);

  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, ""));
  EXPECT_TRUE(rlz_lib::FinancialPing::FormRequest(rlz_lib::TOOLBAR_NOTIFIER,
      points, "swg", brand, "IdOk2", NULL, false, &request));
  expected_response.clear();
  base::StringAppendF(&expected_response,
      "/tools/pso/ping?as=swg&brand=%s&pid=IdOk2&"
      "events=I7S,W1I&rep=2&rlz=T4:&dcc=dcc_value", brand);

  if (got_machine_id)
    base::StringAppendF(&expected_response, "&id=%ls", machine_id.c_str());
  EXPECT_EQ(expected_response, request);

  EXPECT_TRUE(rlz_lib::FinancialPing::FormRequest(rlz_lib::TOOLBAR_NOTIFIER,
      points, "swg", brand, "IdOk", NULL, true, &request));
  expected_response.clear();
  base::StringAppendF(&expected_response,
      "/tools/pso/ping?as=swg&brand=%s&pid=IdOk&"
      "events=I7S,W1I&rep=2&rlz=T4:&dcc=dcc_value", brand);
  EXPECT_EQ(expected_response, request);

  EXPECT_TRUE(rlz_lib::FinancialPing::FormRequest(rlz_lib::TOOLBAR_NOTIFIER,
      points, "swg", brand, NULL, NULL, true, &request));
  expected_response.clear();
  base::StringAppendF(&expected_response,
      "/tools/pso/ping?as=swg&brand=%s&events=I7S,W1I&rep=2"
      "&rlz=T4:&dcc=dcc_value", brand);
  EXPECT_EQ(expected_response, request);


  // Clear all events.
  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));

  // Clear all RLZs.
  char rlz[rlz_lib::kMaxRlzLength + 1];
  int idx = 0;
  for (int ap = rlz_lib::NO_ACCESS_POINT + 1;
       ap < rlz_lib::LAST_ACCESS_POINT; ap++) {
    rlz[0] = 0;
    rlz_lib::AccessPoint point = static_cast<rlz_lib::AccessPoint>(ap);
    if (rlz_lib::GetAccessPointRlz(point, rlz, arraysize(rlz)) && rlz[0]) {
      rlz_lib::SetAccessPointRlz(point, "");
    }
  }

  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX,
      "TbRlzValue"));
  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::QUICK_SEARCH_BOX,
      "QsbRlzValue"));
  EXPECT_TRUE(rlz_lib::FinancialPing::FormRequest(rlz_lib::TOOLBAR_NOTIFIER,
      points, "swg", brand, NULL, NULL, false, &request));
  expected_response.clear();
  base::StringAppendF(&expected_response,
      "/tools/pso/ping?as=swg&brand=%s&rep=2&rlz=T4:TbRlzValue,"
      "Q1:QsbRlzValue&dcc=dcc_value", brand);
  EXPECT_STREQ(expected_response.c_str(), request.c_str());

  if (!GetAccessPointRlz(rlz_lib::IE_HOME_PAGE, rlz, arraysize(rlz))) {
    points[2] = rlz_lib::IE_HOME_PAGE;
    EXPECT_TRUE(rlz_lib::FinancialPing::FormRequest(rlz_lib::TOOLBAR_NOTIFIER,
        points, "swg", brand, "MyId", "en-US", true, &request));
    expected_response.clear();
    base::StringAppendF(&expected_response,
        "/tools/pso/ping?as=swg&brand=%s&hl=en-US&pid=MyId&rep=2"
        "&rlz=T4:TbRlzValue,Q1:QsbRlzValue&dcc=dcc_value", brand);
  EXPECT_STREQ(expected_response.c_str(), request.c_str());
  }
}

TEST_F(FinancialPingTest, FormRequestBadBrand) {
  rlz_lib::AccessPoint points[] =
    {rlz_lib::IETB_SEARCH_BOX, rlz_lib::NO_ACCESS_POINT,
     rlz_lib::NO_ACCESS_POINT};

  std::string request;
  bool ok = rlz_lib::FinancialPing::FormRequest(rlz_lib::TOOLBAR_NOTIFIER,
      points, "swg", "GOOG", NULL, "en", false, &request);
  EXPECT_EQ(rlz_lib::SupplementaryBranding::GetBrand().empty(), ok);
}


static void SetLastPingTime(int64 time, rlz_lib::Product product) {
  const wchar_t* product_name = GetProductName(product);
  base::win::RegKey key;
  rlz_lib::GetPingTimesRegKey(KEY_WRITE, &key);
  EXPECT_EQ(ERROR_SUCCESS, key.WriteValue(product_name, &time, sizeof(time),
                                          REG_QWORD));
}

TEST_F(FinancialPingTest, IsPingTime) {
  int64 now = GetSystemTimeAsInt64();
  int64 last_ping = now - rlz_lib::kEventsPingInterval - k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);

  // No events, last ping just over a day ago.
  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));
  EXPECT_FALSE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                  false));

  // Has events, last ping just over a day ago.
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                 false));

  // Has events, last ping just under a day ago.
  last_ping = now - rlz_lib::kEventsPingInterval + k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);
  EXPECT_FALSE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                  false));

  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));

  // No events, last ping just under a week ago.
  last_ping = now - rlz_lib::kNoEventsPingInterval + k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);
  EXPECT_FALSE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                  false));

  // No events, last ping just over a week ago.
  last_ping = now - rlz_lib::kNoEventsPingInterval - k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);
  EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                 false));

  // Last ping was in future (invalid).
  last_ping = now + k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);
  EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                 false));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                 false));
}

TEST_F(FinancialPingTest, BrandingIsPingTime) {
  // Don't run these tests if a supplementary brand is already in place.  That
  // way we can control the branding.
  if (!rlz_lib::SupplementaryBranding::GetBrand().empty())
    return;

  int64 now = GetSystemTimeAsInt64();
  int64 last_ping = now - rlz_lib::kEventsPingInterval - k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);

  // Has events, last ping just over a day ago.
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                 false));

  {
    rlz_lib::SupplementaryBranding branding(L"TEST");
    SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);

    // Has events, last ping just over a day ago.
    EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
        rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
    EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                   false));
  }

  last_ping = now - k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);

  EXPECT_FALSE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                  false));

  {
    rlz_lib::SupplementaryBranding branding(L"TEST");
    EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                   false));
  }
}

TEST_F(FinancialPingTest, ClearLastPingTime) {
  int64 now = GetSystemTimeAsInt64();
  int64 last_ping = now - rlz_lib::kEventsPingInterval + k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);

  // Has events, last ping just under a day ago.
  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_FALSE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                  false));

  EXPECT_TRUE(rlz_lib::FinancialPing::ClearLastPingTime(
      rlz_lib::TOOLBAR_NOTIFIER));
  EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                 false));
}
