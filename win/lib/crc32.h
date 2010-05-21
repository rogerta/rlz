// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A wrapper around ZLib's CRC function.

#ifndef RLZ_WIN_LIB_CRC32_H_
#define RLZ_WIN_LIB_CRC32_H_

namespace rlz_lib {

int Crc32(const unsigned char* buf, int length);
bool Crc32(const char* text, int* crc);

}  // namespace rlz_lib

#endif  // RLZ_WIN_LIB_CRC32_H_
