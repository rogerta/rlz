// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.

// TODO(thakis): Rename this file to registry_util.
#ifndef RLZ_WIN_LIB_USER_KEY_H_
#define RLZ_WIN_LIB_USER_KEY_H_

namespace base {
namespace win {
class RegKey;
}  // namespace win
}  // namespace base

namespace rlz_lib {

bool RegKeyReadValue(base::win::RegKey& key,
                     const wchar_t* name,
                     char* value,
                     size_t* value_size);

bool RegKeyWriteValue(base::win::RegKey& key,
                      const wchar_t* name,
                      const char* value);

bool HasUserKeyAccess(bool write_access);

}  // namespace rlz_lib

#endif  // RLZ_WIN_LIB_USER_KEY_H_
