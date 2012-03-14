// Copyright 2012 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A library to manage RLZ information for access-points shared
// across different client applications.

#include "rlz/lib/rlz_lib.h"

#include "base/string_util.h"
#include "rlz/lib/assert.h"
#include "rlz/lib/rlz_value_store.h"

namespace {

// Helper functions

bool IsAccessPointSupported(rlz_lib::AccessPoint point) {
  switch (point) {
  case rlz_lib::NO_ACCESS_POINT:
  case rlz_lib::LAST_ACCESS_POINT:

  case rlz_lib::MOBILE_IDLE_SCREEN_BLACKBERRY:
  case rlz_lib::MOBILE_IDLE_SCREEN_WINMOB:
  case rlz_lib::MOBILE_IDLE_SCREEN_SYMBIAN:
    // These AP's are never available on Windows PCs.
    return false;

  case rlz_lib::IE_DEFAULT_SEARCH:
  case rlz_lib::IE_HOME_PAGE:
  case rlz_lib::IETB_SEARCH_BOX:
  case rlz_lib::QUICK_SEARCH_BOX:
  case rlz_lib::GD_DESKBAND:
  case rlz_lib::GD_SEARCH_GADGET:
  case rlz_lib::GD_WEB_SERVER:
  case rlz_lib::GD_OUTLOOK:
  case rlz_lib::CHROME_OMNIBOX:
  case rlz_lib::CHROME_HOME_PAGE:
    // TODO: Figure out when these settings are set to Google.

  default:
    return true;
  }
}

// Current RLZ can only use [a-zA-Z0-9_\-]
// We will be more liberal and allow some additional chars, but not url meta
// chars.
bool IsGoodRlzChar(const char ch) {
  if (IsAsciiAlpha(ch) || IsAsciiDigit(ch))
    return true;

  switch (ch) {
    case '_':
    case '-':
    case '!':
    case '@':
    case '$':
    case '*':
    case '(':
    case ')':
    case ';':
    case '.':
    case '<':
    case '>':
    return true;
  }

  return false;
}

// This function will remove bad rlz chars and also limit the max rlz to some
// reasonable size.  It also assumes that normalized_rlz is at least
// kMaxRlzLength+1 long.
void NormalizeRlz(const char* raw_rlz, char* normalized_rlz) {
  int index = 0;
  for (; raw_rlz[index] != 0 && index < rlz_lib::kMaxRlzLength; ++index) {
    char current = raw_rlz[index];
    if (IsGoodRlzChar(current)) {
      normalized_rlz[index] = current;
    } else {
      normalized_rlz[index] = '.';
    }
  }

  normalized_rlz[index] = 0;
}

}  // namespace

namespace rlz_lib {

// RLZ storage functions.

bool GetAccessPointRlz(AccessPoint point, char* rlz, size_t rlz_size) {
  if (!rlz || rlz_size <= 0) {
    ASSERT_STRING("GetAccessPointRlz: Invalid buffer");
    return false;
  }

  rlz[0] = 0;

  ScopedRlzValueStoreLock lock;
  RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(RlzValueStore::kReadAccess))
    return false;

  if (!IsAccessPointSupported(point))
    return false;

  return store->ReadAccessPointRlz(point, rlz, rlz_size);
}

bool SetAccessPointRlz(AccessPoint point, const char* new_rlz) {
  ScopedRlzValueStoreLock lock;
  RlzValueStore* store = lock.GetStore();
  if (!store || !store->HasAccess(RlzValueStore::kWriteAccess))
    return false;

  if (!new_rlz) {
    ASSERT_STRING("SetAccessPointRlz: Invalid buffer");
    return false;
  }

  // Return false if the access point is not set to Google.
  if (!IsAccessPointSupported(point)) {
    ASSERT_STRING(("SetAccessPointRlz: "
                "Cannot set RLZ for unsupported access point."));
    return false;
  }

  // Verify the RLZ length.
  size_t rlz_length = strlen(new_rlz);
  if (rlz_length > kMaxRlzLength) {
    ASSERT_STRING("SetAccessPointRlz: RLZ length is exceeds max allowed.");
    return false;
  }

  char normalized_rlz[kMaxRlzLength + 1];
  NormalizeRlz(new_rlz, normalized_rlz);
  VERIFY(strlen(new_rlz) == rlz_length);

  // Setting RLZ to empty == clearing.
  if (normalized_rlz[0] == 0)
    return store->ClearAccessPointRlz(point);
  return store->WriteAccessPointRlz(point, normalized_rlz);
}

}  // namespace rlz_lib
