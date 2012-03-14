// Copyright 2012 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.

#ifndef RLZ_MAC_LIB_RLZ_VALUE_STORE_MAC_H_
#define RLZ_MAC_LIB_RLZ_VALUE_STORE_MAC_H_

#include "rlz/lib/rlz_value_store.h"
#include "base/compiler_specific.h"

namespace rlz_lib {

// An implementation of RlzValueStore for mac. For now it's just a stub.
// TODO(thakis): Implement & document, http://crbug.com/117738
class RlzValueStoreMac : public RlzValueStore {
 public:
  virtual bool HasAccess(AccessType type) OVERRIDE;

  virtual bool WritePingTime(Product product, int64 time) OVERRIDE;
  virtual bool ReadPingTime(Product product, int64* time) OVERRIDE;
  virtual bool ClearPingTime(Product product) OVERRIDE;

  virtual bool WriteAccessPointRlz(AccessPoint access_point,
                                   const char* new_rlz) OVERRIDE;
  virtual bool ReadAccessPointRlz(AccessPoint access_point,
                                  char* rlz,
                                  size_t rlz_size) OVERRIDE;
  virtual bool ClearAccessPointRlz(AccessPoint access_point) OVERRIDE;

  virtual bool AddProductEvent(Product product, const char* event_rlz) OVERRIDE;
  virtual bool ClearProductEvent(Product product,
                                 const char* event_rlz) OVERRIDE;

  virtual bool AddStatefulEvent(Product product,
                                const char* event_rlz) OVERRIDE;
  virtual bool IsStatefulEvent(Product product,
                               const char* event_rlz) OVERRIDE;

 private:
  RlzValueStoreMac() {}
  DISALLOW_COPY_AND_ASSIGN(RlzValueStoreMac);
  friend class ScopedRlzValueStoreLock;
};

}  // namespace rlz_lib

#endif  // RLZ_MAC_LIB_RLZ_VALUE_STORE_MAC_H_
