// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// String manipulation functions used in the RLZ library.

#ifndef RLZ_LIB_STRING_UTILS_H_
#define RLZ_LIB_STRING_UTILS_H_

#include "base/string16.h"

namespace base {
namespace win {
class RegKey;
}  // namespace win
}  // namespace base

namespace rlz_lib {

bool IsAscii(char letter);

bool BytesToString(const unsigned char* data,
                   int data_len,
                   string16* string);

bool GetHexValue(char letter, int* value);

int HexStringToInteger(const char* text);

// TODO(thakis): Move these somewhere else.
#if defined(OS_WIN)
bool RegKeyReadValue(base::win::RegKey& key,
                     const wchar_t* name,
                     char* value,
                     size_t* value_size);

bool RegKeyWriteValue(base::win::RegKey& key,
                      const wchar_t* name,
                      const char* value);
#endif

};  // namespace

#endif  // RLZ_LIB_STRING_UTILS_H_
