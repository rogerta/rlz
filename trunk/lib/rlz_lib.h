// Copyright 2012 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A library to manage RLZ information for access-points shared
// across different client applications.
//
// All functions return true on success and false on error.
// This implemenation is thread safe.


#ifndef RLZ_LIB_RLZ_LIB_H_
#define RLZ_LIB_RLZ_LIB_H_

#include <stdio.h>
#include <string>

#include "build/build_config.h"

#include "rlz/lib/rlz_enums.h"

#if defined(OS_WIN)
#define RLZ_LIB_API __cdecl
#else
#define RLZ_LIB_API
#endif

namespace rlz_lib {

// The maximum length of an access points RLZ in bytes.
const int kMaxRlzLength = 64;
// The maximum length of an access points RLZ in bytes.
const int kMaxDccLength = 128;
// The maximum length of a CGI string in bytes.
const int kMaxCgiLength = 2048;
// The maximum length of a ping response we will parse in bytes. If the response
// is bigger, please break it up into separate calls.
const int kMaxPingResponseLength = 0x4000;  // 16K


// RLZ storage functions.

// Get the RLZ value of the access point. If the access point is not Google, the
// RLZ will be the empty string and the function will return false.
// Access: HKCU read.
bool RLZ_LIB_API GetAccessPointRlz(AccessPoint point, char* rlz,
                                   size_t rlz_size);

// Set the RLZ for the access-point. Fails and asserts if called when the access
// point is not set to Google.
// new_rlz should come from a server-response. Client applications should not
// create their own RLZ values.
// Access: HKCU write.
bool RLZ_LIB_API SetAccessPointRlz(AccessPoint point, const char* new_rlz);

// Financial Server pinging functions.

// Pings the financial server and returns the HTTP response. This will fail
// if it is too early to ping the server since the last ping.
//
// product              : The product to ping for.
// request              : The HTTP request (for example, returned by
//                        FormFinancialPingRequest).
// response             : The buffer in which the HTTP response is returned.
// response_buffer_size : The size of the response buffer in bytes. The buffer
//                        size (kMaxPingResponseLength+1) is enough for all
//                        legitimate server responses (any response that is
//                        bigger should be considered the same way as a general
//                        network problem).
//
// Access: HKCU read.
bool RLZ_LIB_API PingFinancialServer(Product product,
                                     const char* request,
                                     char* response,
                                     size_t response_buffer_size);

// Checks if a ping response is valid - ie. it has a checksum line which
// is the CRC-32 checksum of the message uptil the checksum. If
// checksum_idx is not NULL, it will get the index of the checksum, i.e. -
// the effective end of the message.
// Access: No restrictions.
bool RLZ_LIB_API IsPingResponseValid(const char* response,
                                     int* checksum_idx);


// Complex helpers built on top of other functions.

// Copies the events associated with the product and the RLZ's for each access
// point in access_points into cgi. This string can be directly appended
// to a ping (will need an & if not first paramter).
// access_points must be an array of AccessPoints terminated with
// NO_ACCESS_POINT.
// Access: HKCU read.
bool RLZ_LIB_API GetPingParams(Product product,
                               const AccessPoint* access_points,
                               char* unescaped_cgi, size_t unescaped_cgi_size);

#if defined(OS_WIN)
// OEM Deal confirmation storage functions. OEM Deals are windows-only.

// Makes the OEM Deal Confirmation code writable by all users on the machine.
// This should be called before calling SetMachineDealCode from a non-admin
// account.
// Access: HKLM write.
bool RLZ_LIB_API CreateMachineState(void);

// Set the OEM Deal Confirmation Code (DCC). This information is used for RLZ
// initalization.
// Access: HKLM write, or
// HKCU read if rlz_lib::CreateMachineState() has been sucessfully called.
bool RLZ_LIB_API SetMachineDealCode(const char* dcc);

// Get the DCC cgi argument string to append to a daily ping.
// Should be used only by OEM deal trackers. Applications should use the
// GetMachineDealCode method which has an AccessPoint paramter.
// Access: HKLM read.
bool RLZ_LIB_API GetMachineDealCodeAsCgi(char* cgi, size_t cgi_size);

// Get the DCC value stored in registry.
// Should be used only by OEM deal trackers. Applications should use the
// GetMachineDealCode method which has an AccessPoint paramter.
// Access: HKLM read.
bool RLZ_LIB_API GetMachineDealCode(char* dcc, size_t dcc_size);

// Parses the responses from the financial server and updates product state
// and access point RLZ's in registry.
// Access: HKCU write.
bool RLZ_LIB_API ParseFinancialPingResponse(Product product,
                                            const char* response);
#endif



}  // namespace rlz_lib

#endif  // RLZ_LIB_RLZ_LIB_H_
