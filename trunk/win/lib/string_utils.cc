// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// String manipulation functions used in the RLZ library.

#include "rlz/win/lib/string_utils.h"

#include "base/registry.h"
#include "base/string_util.h"
#include "rlz/win/lib/assert.h"

namespace rlz_lib {

bool IsAscii(char letter) {
  return (letter >= 0x0 && letter < 0x80);
}

bool GetHexValue(char letter, int* value) {
  if (!value) {
    ASSERT_STRING("GetHexValue: Invalid output paramter");
    return false;
  }
  *value = 0;

  if (letter >= '0' && letter <= '9')
    *value = letter - '0';
  else if (letter >= 'a' && letter <= 'f')
    *value = (letter - 'a') + 0xA;
  else if (letter >= 'A' && letter <= 'F')
    *value = (letter - 'A') + 0xA;
  else
    return false;

  return true;
}

int HexStringToInteger(const char* text) {
  if (!text) {
    ASSERT_STRING("HexStringToInteger: text is NULL.");
    return 0;
  }

  int idx = 0;
  // Ignore leading whitespace.
  while (text[idx] == '\t' || text[idx] == ' ')
    idx++;

  if ((text[idx] == '0') &&
      (text[idx + 1] == 'X' || text[idx + 1] == 'x'))
    idx +=2;  // String is of the form 0x...

  int number = 0;
  int digit = 0;
  for (; text[idx] != NULL; idx++) {
    if (!GetHexValue(text[idx], &digit)) {
      // Ignore trailing whitespaces, but assert on other trailing characters.
      bool only_whitespaces = true;
      while (only_whitespaces && text[idx])
        only_whitespaces = (text[idx++] == ' ');
      if (!only_whitespaces)
        ASSERT_STRING("HexStringToInteger: text contains non-hex characters.");
      return number;
    }
    number = (number << 4) | digit;
  }

  return number;
}

bool BytesToString(const unsigned char* data,
                   int data_len,
                   std::wstring* string) {
  if (!string)
    return false;

  string->clear();
  if (data_len < 1 || !data)
    return false;

  static const wchar_t* kHex = L"0123456789ABCDEF";

  // Fix the buffer size to begin with to avoid repeated re-allocation.
  string->resize(data_len * 2);
  int index = data_len;
  while (index--) {
    string->at(2 * index) = kHex[data[index] >> 4];  // high digit
    string->at(2 * index + 1) = kHex[data[index] & 0x0F];  // low digit
  }

  return true;
}

bool RegKeyReadValue(RegKey& key, const wchar_t* name,
                     char* value, size_t* value_size) {
  value[0] = 0;

  std::wstring value_string;
  if (!key.ReadValue(name, &value_string)) {
    return false;
  }

  if (value_string.length() > *value_size) {
    *value_size = value_string.length();
    return false;
  }

  strncpy(value, WideToASCII(value_string).c_str(), *value_size);
  value[*value_size - 1] = 0;
  return true;
}

bool RegKeyWriteValue(RegKey& key, const wchar_t* name, const char* value) {
  std::wstring value_string(ASCIIToWide(value));
  return key.WriteValue(name, value_string.c_str());
}

}
