// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Crc8 utility functions.

#ifndef RLZ_WIN_LIB_CRC8_H_
#define RLZ_WIN_LIB_CRC8_H_

namespace rlz_lib {
// CRC-8 methods:
class Crc8 {
 public:
  static bool Generate(const unsigned char* data,
                       int length,
                       unsigned char* check_sum);
  static bool Verify(const unsigned char* data,
                     int length,
                     unsigned char checksum,
                     bool * matches);
};
};  // namespace rlz_lib

#endif  // RLZ_WIN_LIB_CRC8_H_
