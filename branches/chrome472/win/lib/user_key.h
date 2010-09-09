// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A helper library to keep track of a user's key by SID.
// Used by RLZ libary. Also to be used by SearchWithGoogle library.

#ifndef RLZ_WIN_LIB_USER_KEY_H_
#define RLZ_WIN_LIB_USER_KEY_H_

#include "base/registry.h"

namespace rlz_lib {

class UserKey {
 public:
  UserKey(const wchar_t* sid);

  HKEY Get();
  bool HasAccess(bool write_access);
  static bool HasAccess(HKEY user_key, bool write_access);

 private:
  UserKey() {}
  RegKey user_key_;
};
};

#endif  // RLZ_WIN_LIB_USER_KEY_H_
