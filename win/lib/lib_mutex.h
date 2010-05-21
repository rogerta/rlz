// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Mutex to guarantee serialization of RLZ key accesses.

#ifndef RLZ_WIN_LIB_LIB_MUTEX_H_
#define RLZ_WIN_LIB_LIB_MUTEX_H_

#include <windows.h>

namespace rlz_lib {

class LibMutex {
 public:
  LibMutex();
  ~LibMutex();

  bool failed(void) { return !acquired_; }

 private:
  bool acquired_;
  HANDLE mutex_;
};

}  // namespace rlz_lib

#endif  // RLZ_WIN_LIB_LIB_MUTEX_H_
