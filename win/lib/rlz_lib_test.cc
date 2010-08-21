// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A test application for the RLZ library.
//
// These tests should not be executed on the build server:
// - They assert for the failed cases.
// - They modify machine state (registry).
//
// These tests require write access to HKLM and HKCU.

#include "base/logging.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "rlz/win/lib/machine_deal.h"
#include "rlz/win/lib/process_info.h"
#include "rlz/win/lib/rlz_lib.h"

class MachineDealCodeHelper : public rlz_lib::MachineDealCode {
 public:
  static bool Clear() { return rlz_lib::MachineDealCode::Clear(); }

 private:
  MachineDealCodeHelper() {}
  ~MachineDealCodeHelper() {}
};

TEST(RlzLibTest, RecordProductEvent) {
  char cgi_50[50];

  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                             cgi_50, 50));
  EXPECT_STREQ("events=I7S", cgi_50);

  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_HOME_PAGE, rlz_lib::INSTALL));
  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                             cgi_50, 50));
  EXPECT_STREQ("events=I7S,W1I", cgi_50);

  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER, cgi_50, 50));
  EXPECT_STREQ("events=I7S,W1I", cgi_50);
}

TEST(RlzLibTest, ClearProductEvent) {
  char cgi_50[50];

  // Clear 1 of 1 events.
  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                             cgi_50, 50));
  EXPECT_STREQ("events=I7S", cgi_50);
  EXPECT_TRUE(rlz_lib::ClearProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_FALSE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                              cgi_50, 50));
  EXPECT_STREQ("", cgi_50);

  // Clear 1 of 2 events.
  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_HOME_PAGE, rlz_lib::INSTALL));
  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                             cgi_50, 50));
  EXPECT_STREQ("events=I7S,W1I", cgi_50);
  EXPECT_TRUE(rlz_lib::ClearProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                             cgi_50, 50));
  EXPECT_STREQ("events=W1I", cgi_50);

  // Clear a non-recorded event.
  EXPECT_TRUE(rlz_lib::ClearProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IETB_SEARCH_BOX, rlz_lib::FIRST_SEARCH));
  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                             cgi_50, 50));
  EXPECT_STREQ("events=W1I", cgi_50);
}


TEST(RlzLibTest, GetProductEventsAsCgi) {
  char cgi_50[50];
  char cgi_1[1];

  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                             cgi_50, 50));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_HOME_PAGE, rlz_lib::INSTALL));

  EXPECT_FALSE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                              cgi_1, 1));
  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                             cgi_50, 50));
  EXPECT_STREQ("events=I7S,W1I", cgi_50);
}

TEST(RlzLibTest, ClearAllAllProductEvents) {
  char cgi_50[50];

  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                             cgi_50, 50));
  EXPECT_STREQ("events=I7S", cgi_50);

  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));
  EXPECT_FALSE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                              cgi_50, 50));
  EXPECT_STREQ("", cgi_50);
}

TEST(RlzLibTest, SetAccessPointRlz) {
  char rlz_50[50];
  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, ""));
  EXPECT_TRUE(rlz_lib::GetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, rlz_50, 50));
  EXPECT_STREQ("", rlz_50);

  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, "IeTbRlz"));
  EXPECT_TRUE(rlz_lib::GetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, rlz_50, 50));
  EXPECT_STREQ("IeTbRlz", rlz_50);
}

TEST(RlzLibTest, GetAccessPointRlz) {
  char rlz_1[1];
  char rlz_50[50];
  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, ""));
  EXPECT_TRUE(rlz_lib::GetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, rlz_1, 1));
  EXPECT_STREQ("", rlz_1);

  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, "IeTbRlz"));
  EXPECT_FALSE(rlz_lib::GetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, rlz_1, 1));
  EXPECT_TRUE(rlz_lib::GetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, rlz_50, 50));
  EXPECT_STREQ("IeTbRlz", rlz_50);
}

TEST(RlzLibTest, GetPingParams) {
  if (rlz_lib::ProcessInfo::HasAdminRights()) {
    MachineDealCodeHelper::Clear();

    EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX,
        "TbRlzValue"));
    EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IE_HOME_PAGE, ""));

    char cgi[2048];
    rlz_lib::AccessPoint points[] =
      {rlz_lib::IETB_SEARCH_BOX, rlz_lib::NO_ACCESS_POINT,
       rlz_lib::NO_ACCESS_POINT};

    EXPECT_TRUE(rlz_lib::GetPingParams(rlz_lib::TOOLBAR_NOTIFIER, points,
                                       cgi, 2048));
    EXPECT_STREQ("rep=2&rlz=T4:TbRlzValue", cgi);

    EXPECT_TRUE(rlz_lib::MachineDealCode::Set("dcc_value"));
    EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, ""));
    EXPECT_TRUE(rlz_lib::GetPingParams(rlz_lib::TOOLBAR_NOTIFIER, points,
                                       cgi, 2048));
    EXPECT_STREQ("rep=2&rlz=T4:&dcc=dcc_value", cgi);

    EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX,
                "TbRlzValue"));
    EXPECT_FALSE(rlz_lib::GetPingParams(rlz_lib::TOOLBAR_NOTIFIER, points,
                                        cgi, 37));
    EXPECT_STREQ("", cgi);
    EXPECT_TRUE(rlz_lib::GetPingParams(rlz_lib::TOOLBAR_NOTIFIER, points,
                                       cgi, 38));
    EXPECT_STREQ("rep=2&rlz=T4:TbRlzValue&dcc=dcc_value", cgi);

    EXPECT_TRUE(GetAccessPointRlz(rlz_lib::IE_HOME_PAGE, cgi, 2048));
    points[2] = rlz_lib::IE_HOME_PAGE;
    EXPECT_TRUE(rlz_lib::GetPingParams(rlz_lib::TOOLBAR_NOTIFIER, points,
                                       cgi, 2048));
    EXPECT_STREQ("rep=2&rlz=T4:TbRlzValue&dcc=dcc_value", cgi);
  } else {
    LOG(ERROR) <<
        "\n\n *** Please re-run the unit tests with administrator privileges\n"
        " *** to see the results of this test.\n";
  }
}

TEST(RlzLibTest, IsPingResponseValid) {
  const char* kBadPingResponses[] = {
    // No checksum.
    "version: 3.0.914.7250\r\n"
    "url: http://www.corp.google.com/~av/45/opt/SearchWithGoogleUpdate.exe\r\n"
    "launch-action: custom-action\r\n"
    "launch-target: SearchWithGoogleUpdate.exe\r\n"
    "signature: c08a3f4438e1442c4fe5678ee147cf6c5516e5d62bb64e\r\n"
    "rlz: 1R1_____en__252\r\n"
    "rlzXX: 1R1_____en__250\r\n",

    // Invalid checksum.
    "version: 3.0.914.7250\r\n"
    "url: http://www.corp.google.com/~av/45/opt/SearchWithGoogleUpdate.exe\r\n"
    "launch-action: custom-action\r\n"
    "launch-target: SearchWithGoogleUpdate.exe\r\n"
    "signature: c08a3f4438e1442c4fe5678ee147cf6c5516e5d62bb64e\r\n"
    "rlz: 1R1_____en__252\r\n"
    "rlzXX: 1R1_____en__250\r\n"
    "rlzT4  1T4_____en__251\r\n"
    "rlzT4: 1T4_____en__252\r\n"
    "rlz\r\n"
    "crc32: B12CC79A",

    // Misplaced checksum.
    "version: 3.0.914.7250\r\n"
    "url: http://www.corp.google.com/~av/45/opt/SearchWithGoogleUpdate.exe\r\n"
    "launch-action: custom-action\r\n"
    "launch-target: SearchWithGoogleUpdate.exe\r\n"
    "signature: c08a3f4438e1442c4fe5678ee147cf6c5516e5d62bb64e\r\n"
    "rlz: 1R1_____en__252\r\n"
    "rlzXX: 1R1_____en__250\r\n"
    "crc32: B12CC79C\r\n"
    "rlzT4  1T4_____en__251\r\n"
    "rlzT4: 1T4_____en__252\r\n"
    "rlz\r\n",

    NULL
  };

  const char* kGoodPingResponses[] = {
    "version: 3.0.914.7250\r\n"
    "url: http://www.corp.google.com/~av/45/opt/SearchWithGoogleUpdate.exe\r\n"
    "launch-action: custom-action\r\n"
    "launch-target: SearchWithGoogleUpdate.exe\r\n"
    "signature: c08a3f4438e1442c4fe5678ee147cf6c5516e5d62bb64e\r\n"
    "rlz: 1R1_____en__252\r\n"
    "rlzXX: 1R1_____en__250\r\n"
    "rlzT4  1T4_____en__251\r\n"
    "rlzT4: 1T4_____en__252\r\n"
    "rlz\r\n"
    "crc32: D6FD55A3",

    "version: 3.0.914.7250\r\n"
    "url: http://www.corp.google.com/~av/45/opt/SearchWithGoogleUpdate.exe\r\n"
    "launch-action: custom-action\r\n"
    "launch-target: SearchWithGoogleUpdate.exe\r\n"
    "signature: c08a3f4438e1442c4fe5678ee147cf6c5516e5d62bb64e\r\n"
    "rlz: 1R1_____en__252\r\n"
    "rlzXX: 1R1_____en__250\r\n"
    "rlzT4  1T4_____en__251\r\n"
    "rlzT4: 1T4_____en__252\r\n"
    "rlz\r\n"
    "crc32: D6FD55A3\r\n"
    "extradata: not checksummed",

    NULL
  };

  for (int i = 0; kBadPingResponses[i]; i++)
    EXPECT_FALSE(rlz_lib::IsPingResponseValid(kBadPingResponses[i], NULL));

  for (int i = 0; kGoodPingResponses[i]; i++)
    EXPECT_TRUE(rlz_lib::IsPingResponseValid(kGoodPingResponses[i], NULL));
}

TEST(RlzLibTest, ParsePingResponse) {
  const char* kPingResponse =
    "version: 3.0.914.7250\r\n"
    "url: http://www.corp.google.com/~av/45/opt/SearchWithGoogleUpdate.exe\r\n"
    "launch-action: custom-action\r\n"
    "launch-target: SearchWithGoogleUpdate.exe\r\n"
    "signature: c08a3f4438e1442c4fe5678ee147cf6c5516e5d62bb64e\r\n"
    "rlz: 1R1_____en__252\r\n"  // Invalid RLZ - no access point.
    "rlzXX: 1R1_____en__250\r\n"  // Invalid RLZ - bad access point.
    "rlzT4  1T4_____en__251\r\n"  // Invalid RLZ - missing colon.
    "rlzT4: 1T4_____en__252\r\n"  // GoodRLZ.
    "events: I7S,W1I\r\n"  // Clear all events.
    "rlz\r\n"
    "dcc: dcc_value\r\n"
    "crc32: F9070F81";

  // Only run this test if we are admin, otherwise it will surely fail.
  if (rlz_lib::ProcessInfo::HasAdminRights()) {
    EXPECT_TRUE(rlz_lib::MachineDealCode::Set("dcc_value2"));

    // Record some product events to check that they get cleared.
    EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
        rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
    EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
        rlz_lib::IE_HOME_PAGE, rlz_lib::INSTALL));

    EXPECT_TRUE(rlz_lib::SetAccessPointRlz(
        rlz_lib::IETB_SEARCH_BOX, "TbRlzValue"));

    EXPECT_TRUE(rlz_lib::ParsePingResponse(rlz_lib::TOOLBAR_NOTIFIER,
                                           kPingResponse));

    EXPECT_TRUE(rlz_lib::MachineDealCode::Set("dcc_value"));
    EXPECT_TRUE(rlz_lib::ParsePingResponse(rlz_lib::TOOLBAR_NOTIFIER,
                                           kPingResponse));

    char value[50];
    EXPECT_TRUE(rlz_lib::GetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, value, 50));
    EXPECT_STREQ("1T4_____en__252", value);
    EXPECT_FALSE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                                value, 50));
    EXPECT_STREQ("", value);

    const char* kPingResponse2 =
      "rlzT4:    1T4_____de__253  \r\n"  // Good with extra spaces.
      "crc32: 321334F5\r\n";
    EXPECT_TRUE(rlz_lib::ParsePingResponse(rlz_lib::TOOLBAR_NOTIFIER,
                                           kPingResponse2));
    EXPECT_TRUE(rlz_lib::GetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX, value, 50));
    EXPECT_STREQ("1T4_____de__253", value);

    const char* kPingResponse3 =
      "crc32: 0\r\n";  // Good RLZ - empty response.
    EXPECT_TRUE(rlz_lib::ParsePingResponse(rlz_lib::TOOLBAR_NOTIFIER,
                                           kPingResponse3));
    EXPECT_STREQ("1T4_____de__253", value);
  } else {
    LOG(ERROR) <<
        "\n\n *** Please re-run the unit tests with administrator privileges\n"
        " *** to see the results of this test.\n";
  }
}

// Test whether a stateful event will only be sent in financial pings once.
TEST(RlzLibTest, ParsePingResponseWithStatefulEvents) {
  const char* kPingResponse =
    "version: 3.0.914.7250\r\n"
    "url: http://www.corp.google.com/~av/45/opt/SearchWithGoogleUpdate.exe\r\n"
    "launch-action: custom-action\r\n"
    "launch-target: SearchWithGoogleUpdate.exe\r\n"
    "signature: c08a3f4438e1442c4fe5678ee147cf6c5516e5d62bb64e\r\n"
    "rlzT4: 1T4_____en__252\r\n"  // GoodRLZ.
    "events: I7S,W1I\r\n"         // Clear all events.
    "stateful-events: W1I\r\n"    // W1I as an stateful event.
    "rlz\r\n"
    "dcc: dcc_value\r\n"
    "crc32: 55191759";

  EXPECT_TRUE(rlz_lib::ClearAllProductEvents(rlz_lib::TOOLBAR_NOTIFIER));

  // Record some product events to check that they get cleared.
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_HOME_PAGE, rlz_lib::INSTALL));

  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(
      rlz_lib::IETB_SEARCH_BOX, "TbRlzValue"));

  EXPECT_TRUE(rlz_lib::ParsePingResponse(rlz_lib::TOOLBAR_NOTIFIER,
                                         kPingResponse));

  // Check all the events sent earlier are cleared.
  char value[50];
  EXPECT_FALSE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                              value, 50));
  EXPECT_STREQ("", value);

  // Record both events (one is stateless and the other is stateful) again.
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_HOME_PAGE, rlz_lib::INSTALL));

  // Check the stateful event won't be sent again while the stateless one will.
  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                             value, 50));
  EXPECT_STREQ("events=I7S", value);
}

TEST(RlzLibTest, SendFinancialPing) {
  // Only run this test if we are admin, otherwise it will surely fail.
  if (rlz_lib::ProcessInfo::HasAdminRights()) {
    // We don't really check a value or result in this test. All this does is
    // attempt to ping the financial server, which you can verify in Fiddler.
    // TODO: Make this a measurable test.
    MachineDealCodeHelper::Clear();
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

    std::string request;
    rlz_lib::SendFinancialPing(rlz_lib::TOOLBAR_NOTIFIER, points,
        "swg", "GGLA", "SwgProductId1234", "en-UK", false);
  } else {
    LOG(ERROR) <<
        "\n\n *** Please re-run the unit tests with administrator privileges\n"
        " *** to see the results of this test.\n";
  }
}

TEST(RlzLibTest, ClearProductState) {
  MachineDealCodeHelper::Clear();

  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX,
      "TbRlzValue"));
  EXPECT_TRUE(rlz_lib::SetAccessPointRlz(rlz_lib::GD_DESKBAND,
      "GdbRlzValue"));

  rlz_lib::AccessPoint points[] =
      { rlz_lib::IETB_SEARCH_BOX, rlz_lib::NO_ACCESS_POINT };

  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IE_DEFAULT_SEARCH, rlz_lib::SET_TO_GOOGLE));
  EXPECT_TRUE(rlz_lib::RecordProductEvent(rlz_lib::TOOLBAR_NOTIFIER,
      rlz_lib::IETB_SEARCH_BOX, rlz_lib::INSTALL));

  rlz_lib::AccessPoint points2[] =
    { rlz_lib::IETB_SEARCH_BOX,
      rlz_lib::GD_DESKBAND,
      rlz_lib::NO_ACCESS_POINT };

  char cgi[2048];
  EXPECT_TRUE(rlz_lib::GetPingParams(rlz_lib::TOOLBAR_NOTIFIER, points2,
                                     cgi, 2048));
  EXPECT_STREQ("rep=2&rlz=T4:TbRlzValue,D1:GdbRlzValue", cgi);

  EXPECT_TRUE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                             cgi, 2048));
  std::string events(cgi);
  EXPECT_LT(0u, events.find("I7S"));
  EXPECT_LT(0u, events.find("T4I"));
  EXPECT_LT(0u, events.find("T4R"));

  rlz_lib::ClearProductState(rlz_lib::TOOLBAR_NOTIFIER, points, NULL);

  EXPECT_TRUE(rlz_lib::GetAccessPointRlz(rlz_lib::IETB_SEARCH_BOX,
                                         cgi, 2048));
  EXPECT_STREQ("", cgi);
  EXPECT_TRUE(rlz_lib::GetAccessPointRlz(rlz_lib::GD_DESKBAND,
                                         cgi, 2048));
  EXPECT_STREQ("GdbRlzValue", cgi);

  EXPECT_FALSE(rlz_lib::GetProductEventsAsCgi(rlz_lib::TOOLBAR_NOTIFIER,
                                              cgi, 2048));
  EXPECT_STREQ("", cgi);
}
