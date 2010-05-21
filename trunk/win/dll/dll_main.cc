// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Main function for the RLZ DLL.

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>

BOOL APIENTRY DllMain(HANDLE module, DWORD  reason, LPVOID reserved) {
  return TRUE;
}

