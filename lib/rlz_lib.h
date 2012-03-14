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

}  // namespace rlz_lib

#endif  // RLZ_LIB_RLZ_LIB_H_
