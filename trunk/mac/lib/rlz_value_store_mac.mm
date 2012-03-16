// Copyright 2012 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.

#include "rlz/mac/lib/rlz_value_store_mac.h"

#include "base/logging.h"

namespace rlz_lib {

bool RlzValueStoreMac::HasAccess(AccessType type) {
  return true;
}

bool RlzValueStoreMac::ReadPingTime(Product product, int64* time) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::WritePingTime(Product product, int64 time) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::ClearPingTime(Product product) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::WriteAccessPointRlz(AccessPoint access_point,
                                                const char* new_rlz) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::ReadAccessPointRlz(AccessPoint access_point,
                                               char* rlz,
                                               size_t rlz_size) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::ClearAccessPointRlz(AccessPoint access_point) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::AddProductEvent(Product product,
                                       const char* event_rlz) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::ReadProductEvents(Product product,
                                         std::vector<std::string>* events) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::ClearProductEvent(Product product,
                                         const char* event_rlz) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::ClearAllProductEvents(Product product) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::AddStatefulEvent(Product product,
                                        const char* event_rlz) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::IsStatefulEvent(Product product,
                                       const char* event_rlz) {
  NOTIMPLEMENTED();
  return false;
}

bool RlzValueStoreMac::ClearAllStatefulEvents(Product product) {
  NOTIMPLEMENTED();
  return false;
}

void RlzValueStoreMac::CollectGarbage() {
  NOTIMPLEMENTED();
}

ScopedRlzValueStoreLock::ScopedRlzValueStoreLock() {
  // TODO(thakis): Figure out locking.
  store_.reset(new RlzValueStoreMac);
}

ScopedRlzValueStoreLock::~ScopedRlzValueStoreLock() {
}

RlzValueStore* ScopedRlzValueStoreLock::GetStore() {
  return store_.get();
}

}  // namespace rlz_lib
