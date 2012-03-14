// Copyright 2012 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.

#ifndef RLZ_VALUE_STORE_H_
#define RLZ_VALUE_STORE_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "rlz/lib/rlz_enums.h"

#if defined(OS_WIN)
#include "rlz/win/lib/lib_mutex.h"
#endif

namespace rlz_lib {

// Abstracts away rlz's key value store. On windows, this usually writes to
// the registry. On mac, it writes to an NSDefaults object.
class RlzValueStore {
 public:
  virtual ~RlzValueStore() {}

  enum AccessType { kReadAccess, kWriteAccess };
  virtual bool HasAccess(AccessType type) = 0;

  // Ping times.
  virtual bool WritePingTime(Product product, int64 time) = 0;
  virtual bool ReadPingTime(Product product, int64* time) = 0;
  virtual bool ClearPingTime(Product product) = 0;

  // Access point RLZs.
  virtual bool WriteAccessPointRlz(AccessPoint access_point,
                                   const char* new_rlz) = 0;
  virtual bool ReadAccessPointRlz(AccessPoint access_point,
                                  char* rlz,  // At most kMaxRlzLength + 1 bytes
                                  size_t rlz_size) = 0;
  virtual bool ClearAccessPointRlz(AccessPoint access_point) = 0;
};

// All methods of RlzValueStore must stays consistent even when accessed from
// multiple threads in multiple processes. To enforce this through the type
// system, the only way to access the RlzValueStore is through a
// ScopedRlzValueStoreLock, which is a cross-process lock. It is active while
// it is in scope. If the class fails to acquire a lock, its GetStore() method
// returns NULL.
class ScopedRlzValueStoreLock {
 public:
  ScopedRlzValueStoreLock();
  ~ScopedRlzValueStoreLock();

  // Returns a RlzValueStore protected by a cross-process lock, or NULL if the
  // lock can't be obtained. The lifetime of the returned object is limited to
  // the lifetime of this ScopedRlzValueStoreLock object.
  RlzValueStore* GetStore();

 private:
  scoped_ptr<RlzValueStore> store_;
#if defined(OS_WIN)
  LibMutex lock_;
#else
  // TODO(thakis): Mac implementation
#endif
};

}

#endif  // RLZ_VALUE_STORE_H_
