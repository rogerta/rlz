// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Library functions related to the Financial Server ping.

#ifndef RLZ_WIN_LIB_FINANCIAL_PING_H_
#define RLZ_WIN_LIB_FINANCIAL_PING_H_

#include <string>
#include "rlz/win/lib/rlz_lib.h"

namespace rlz_lib {

class FinancialPing {
 public:
  // Form the HTTP request to send to the PSO server.
  // Will look something like:
  // /pso/ping?as=swg&brand=GGLD&id=124&hl=en&
  //           events=I7S&rep=1&rlz=I7:val,W1:&dcc=dval
  static bool FormRequest(Product product, const AccessPoint* access_points,
                          const char* product_signature,
                          const char* product_brand, const char* product_id,
                          const char* product_lang, bool exclude_machine_id,
                          std::string* request);

  // Returns whether the time is right to send a ping.
  // If no_delay is true, this should always ping if there are events,
  // or one week has passed since last_ping when there are no new events.
  // If no_delay is false, this should ping if current time < last_ping time
  // (case of time reset) or if one day has passed since last_ping and there
  // are events, or one week has passed since last_ping when there are
  // no new events.
  static bool IsPingTime(Product product, const wchar_t* sid, bool no_delay);

  // Set the last ping time to be now. Writes to HKCU.
  static bool UpdateLastPingTime(Product product, const wchar_t* sid);

  // Clear the last ping time - should be called on uninstall. Writes to HKCU.
  static bool ClearLastPingTime(Product product, const wchar_t* sid);

  // Ping the financial server with request. Writes to HKCU.
  static bool PingServer(const char* request, std::string* response);

 private:
  FinancialPing() {}
  ~FinancialPing() {}
};

}  // namespace rlz_lib


#endif  // RLZ_WIN_LIB_FINANCIAL_PING_H_
