# Copyright 2010 Google Inc. All Rights Reserved.
# Use of this source code is governed by an Apache-style license that can be
# found in the COPYING file.

deps = {
  "src/base":
    "http://src.chromium.org/svn/trunk/src/base@55888",

  "src/build":
    "http://src.chromium.org/svn/trunk/src/build@55888",

  "src/third_party/icu":
    "http://src.chromium.org/svn/trunk/deps/third_party/icu42@43139",

  "src/third_party/modp_b64":
    "http://src.chromium.org/svn/trunk/src/third_party/modp_b64@55888",

  "src/third_party/nss":
    "http://src.chromium.org/svn/trunk/deps/third_party/nss@45059",

  "src/third_party/sqlite":
    "http://src.chromium.org/svn/trunk/src/third_party/sqlite@55888",

  "src/third_party/wtl":
    "http://src.chromium.org/svn/trunk/src/third_party/wtl@55888",

  "src/third_party/zlib":
    "http://src.chromium.org/svn/trunk/src/third_party/zlib@55888",

  "src/testing":
    "http://src.chromium.org/svn/trunk/src/testing@55888",

  "src/testing/gtest":
    "http://googletest.googlecode.com/svn/trunk@408",

  "src/tools/gyp":
    "http://gyp.googlecode.com/svn/trunk@818",
}

include_rules = [
  "+base",
  "+build",
]

hooks = [
  {
    # A change to a .gyp, .gypi, or to GYP itself should run the generator.
    "pattern": ".",
    "action": ["python", "src/build/gyp_chromium", "src/rlz/rlz.gyp"],
  }
]

