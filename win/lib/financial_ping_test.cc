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

#include <windows.h>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "base/win/registry.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "rlz/win/lib/financial_ping.h"
#include "rlz/win/lib/lib_values.h"
#include "rlz/win/lib/machine_deal.h"
#include "rlz/win/test/rlz_test_helpers.h"

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
      points, "swg", "GGLA", NULL, "en", false, NULL, &request));
  std::string expected_response =
      "/tools/pso/ping?as=swg&brand=GGLA&hl=en&"
      "events=I7S,W1I&rep=2&rlz=T4:TbRlzValue&dcc=dcc_value";
  if (got_machine_id)
    base::StringAppendF(&expected_response, "&id=%ls", machine_id.c_str());
  EXPECT_EQ(expected_response, request);

  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, ""));
  EXPECT_TRUE(rlz_lib::FinancialPing::FormRequest(rlz_lib::TOOLBAR_NOTIFIER,
      points, "swg", "GGLA", "IdOk2", NULL, false, NULL, &request));
  expected_response = "/tools/pso/ping?as=swg&brand=GGLA&pid=IdOk2&"
                      "events=I7S,W1I&rep=2&rlz=T4:&dcc=dcc_value";
  if (got_machine_id)
    base::StringAppendF(&expected_response, "&id=%ls", machine_id.c_str());
  EXPECT_EQ(expected_response, request);

  EXPECT_TRUE(rlz_lib::FinancialPing::FormRequest(rlz_lib::TOOLBAR_NOTIFIER,
      points, "swg", "GGLA", "IdOk", NULL, true, NULL, &request));
  expected_response = "/tools/pso/ping?as=swg&brand=GGLA&pid=IdOk&"
                      "events=I7S,W1I&rep=2&rlz=T4:&dcc=dcc_value";
  EXPECT_EQ(expected_response, request);

  EXPECT_TRUE(rlz_lib::FinancialPing::FormRequest(rlz_lib::TOOLBAR_NOTIFIER,
      points, "swg", "GGLA", NULL, NULL, true, NULL, &request));
  expected_response = "/tools/pso/ping?as=swg&brand=GGLA&events=I7S,W1I&rep=2"
                      "&rlz=T4:&dcc=dcc_value";
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
      points, "swg", "GGLA", NULL, NULL, false, NULL, &request));
  EXPECT_STREQ("/tools/pso/ping?as=swg&brand=GGLA&rep=2&rlz=T4:TbRlzValue,"
               "Q1:QsbRlzValue&dcc=dcc_value",
               request.c_str());

  if (!GetAccessPointRlz(rlz_lib::IE_HOME_PAGE, rlz, arraysize(rlz))) {
    points[2] = rlz_lib::IE_HOME_PAGE;
    EXPECT_TRUE(rlz_lib::FinancialPing::FormRequest(rlz_lib::TOOLBAR_NOTIFIER,
        points, "swg", "GGLA", "MyId", "en-US", true, NULL, &request));
    EXPECT_STREQ("/tools/pso/ping?as=swg&brand=GGLA&hl=en-US&pid=MyId&rep=2"
                 "&rlz=T4:TbRlzValue,Q1:QsbRlzValue&dcc=dcc_value",
                 request.c_str());
  }
}


static void SetLastPingTime(int64 time, rlz_lib::Product product) {
  std::wstring key_location;
  base::StringAppendF(&key_location, L"%ls\\%ls", rlz_lib::kLibKeyName,
                      rlz_lib::kPingTimesSubkeyName);

  const wchar_t* product_name = GetProductName(product);
  base::win::RegKey key(HKEY_CURRENT_USER, key_location.c_str(), KEY_WRITE);
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
                                                  NULL, false));

  // Has events, last ping just over a day ago.
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                  NULL, false));

  // Has events, last ping just under a day ago.
  last_ping = now - rlz_lib::kEventsPingInterval + k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);
  EXPECT_FALSE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                  NULL, false));

  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));

  // No events, last ping just under a week ago.
  last_ping = now - rlz_lib::kNoEventsPingInterval + k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);
  EXPECT_FALSE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                  NULL, false));

  // No events, last ping just over a week ago.
  last_ping = now - rlz_lib::kNoEventsPingInterval - k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);
  EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                  NULL, false));

  // Last ping was in future (invalid).
  last_ping = now + k1MinuteInterval;
  SetLastPingTime(last_ping, rlz_lib::TOOLBAR_NOTIFIER);
  EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                  NULL, false));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                  NULL, false));
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
                                                  NULL, false));

  EXPECT_TRUE(rlz_lib::FinancialPing::ClearLastPingTime(
      rlz_lib::TOOLBAR_NOTIFIER, NULL));
  EXPECT_TRUE(rlz_lib::FinancialPing::IsPingTime(rlz_lib::TOOLBAR_NOTIFIER,
                                                 NULL, false));
}
