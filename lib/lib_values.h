// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Key and value names of the location of the RLZ shared state.

#ifndef RLZ_LIB_LIB_VALUES_H_
#define RLZ_LIB_LIB_VALUES_H_

#include "base/basictypes.h"
#include "rlz/win/lib/rlz_lib.h"

#if defined(OS_WIN)
#include "base/win/registry.h"
#endif

namespace rlz_lib {

// TODO(thakis): Move registry stuff somewhere else.
#if defined(OS_WIN)
//
// Registry keys:
//
//   RLZ's are stored as:
//   <AccessPointName>  = <RLZ value> @ kRootKey\kLibKeyName\kRlzsSubkeyName.
//
//   Events are stored as:
//   <AccessPointName><EventName> = 1 @
//   HKCU\kLibKeyName\kEventsSubkeyName\GetProductName(product).
//
//   The OEM Deal Confirmation Code (DCC) is stored as
//   kDccValueName = <DCC value> @ HKLM\kLibKeyName
//
//   The last ping time, per product is stored as:
//   GetProductName(product) = <last ping time> @
//   HKCU\kLibKeyName\kPingTimesSubkeyName.
//
// The server does not care about any of these constants.
//
extern const wchar_t kLibKeyName[];
extern const wchar_t kRlzsSubkeyName[];
extern const wchar_t kEventsSubkeyName[];
extern const wchar_t kStatefulEventsSubkeyName[];
extern const wchar_t kDccValueName[];
extern const wchar_t kPingTimesSubkeyName[];

const wchar_t* GetProductName(Product product);

// These are the parent keys of kLibKeyName. These can be deleted safely
// if completely empty.
extern const wchar_t kGoogleKeyName[];
extern const wchar_t kGoogleCommonKeyName[];

// Function to get the specific registry keys.
bool GetPingTimesRegKey(REGSAM access,
                        base::win::RegKey* key);

bool GetEventsRegKey(const wchar_t* event_type,
                     const rlz_lib::Product* product,
                     REGSAM access,
                     base::win::RegKey* key);

bool GetAccessPointRlzsRegKey(REGSAM access,
                              base::win::RegKey* key);

void AppendBrandToString(std::wstring* str);
#endif  // defined(OS_WIN)


//
// Ping CGI arguments:
//
//   Events are reported as (without spaces):
//   kEventsCgiVariable = <AccessPoint1><Event1> kEventsCgiSeparator <P2><E2>...
//
//   Event responses from the server look like:
//   kEventsCgiVariable : <AccessPoint1><Event1> kEventsCgiSeparator <P2><E2>...
//
//   RLZ's are reported as (without spaces):
//   kRlzCgiVariable = <AccessPoint> <kRlzCgiIndicator> <RLZ value>
//        <kRlzCgiSeparator> <AP2><Indicator><V2><Separator> ....
//
//   RLZ responses from the server look like (without spaces):
//   kRlzCgiVariable<Access Point> :  <RLZ value>
//
//   DCC if reported should look like (without spaces):
//   kDccCgiVariable = <DCC Value>
//
//   RLS if reported should look like (without spaces):
//   kRlsCgiVariable = <RLS Value>
//
//   Machine ID if reported should look like (without spaces):
//   kMachineIdCgiVariable = <Machine ID Value>
//
//   A server response setting / confirming the DCC will look like (no spaces):
//   kDccCgiVariable : <DCC Value>
//
//   Each ping to the server must also contain kProtocolCgiArgument as well.
//
//   Pings may also contain (but not necessarily controlled by this Lib):
//   - The product signature: kProductSignatureCgiVariable = <signature>
//   - The product brand: kProductBrandCgiVariable = <brand>
//   - The product installation ID: kProductIdCgiVariable = <id>
extern const char kEventsCgiVariable[];
extern const char kStatefulEventsCgiVariable[];
extern const char kEventsCgiSeparator;

extern const char kDccCgiVariable[];
extern const char kProtocolCgiArgument[];

extern const char kProductSignatureCgiVariable[];
extern const char kProductBrandCgiVariable[];
extern const char kProductLanguageCgiVariable[];
extern const char kProductIdCgiVariable[];

extern const char kRlzCgiVariable[];
extern const char kRlzCgiSeparator[];
extern const char kRlzCgiIndicator[];

extern const char kRlsCgiVariable[];
extern const char kMachineIdCgiVariable[];
extern const char kSetDccResponseVariable[];

//
// Financial ping server information.
//

extern const char kFinancialPingPath[];
extern const char kFinancialServer[];
extern const char kFinancialPingType[];

extern const int kFinancialPort;

extern const int64 kEventsPingInterval;
extern const int64 kNoEventsPingInterval;

extern const char kFinancialPingUserAgent[];
extern const char* kFinancialPingResponseObjects[];

//
// The names for AccessPoints and Events that we use MUST be the same
// as those used/understood by the server.
//
const char* GetAccessPointName(AccessPoint point);
bool GetAccessPointFromName(const char* name, AccessPoint* point);

const char* GetEventName(Event event);
bool GetEventFromName(const char* name, Event* event);

}  // namespace rlz_lib

#endif  // RLZ_LIB_LIB_VALUES_H_
