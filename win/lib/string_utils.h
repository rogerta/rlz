// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// String manipulation functions used in the RLZ library.

#ifndef RLZ_WIN_LIB_STRING_UTILS_H_
#define RLZ_WIN_LIB_STRING_UTILS_H_

#include <string>

namespace base {
namespace win {
class RegKey;
}  // namespace win
}  // namespace base

namespace rlz_lib {

bool IsAscii(char letter);

bool BytesToString(const unsigned char* data,
                   int data_len,
                   std::wstring* string);

bool GetHexValue(char letter, int* value);

int HexStringToInteger(const char* text);

bool RegKeyReadValue(base::win::RegKey& key,
                     const wchar_t* name,
                     char* value,
                     size_t* value_size);

bool RegKeyWriteValue(base::win::RegKey& key,
                      const wchar_t* name,
                      const char* value);

};  // namespace

#endif  // RLZ_WIN_LIB_STRING_UTILS_H_
