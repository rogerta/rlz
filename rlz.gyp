# Copyright 2011 Google Inc. All Rights Reserved.
# Use of this source code is governed by an Apache-style license that can be
# found in the COPYING file.

{
  'target_defaults': {
    'include_dirs': [
      '..',
    ],
  },
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'rlz_lib',
          'type': 'static_library',
          'include_dirs': [],
          'sources': [
            'win/lib/assert.cc',
            'win/lib/assert.h',
            'win/lib/crc32.h',
            'win/lib/crc32_wrapper.cc',
            'win/lib/crc8.h',
            'win/lib/crc8.cc',
            'win/lib/financial_ping.cc',
            'win/lib/financial_ping.h',
            'win/lib/lib_mutex.cc',
            'win/lib/lib_mutex.h',
            'win/lib/lib_values.cc',
            'win/lib/lib_values.h',
            'win/lib/machine_deal.cc',
            'win/lib/machine_deal.h',
            'win/lib/process_info.cc',
            'win/lib/process_info.h',
            'win/lib/rlz_lib.cc',
            'win/lib/rlz_lib.h',
            'win/lib/string_utils.cc',
            'win/lib/string_utils.h',
            'win/lib/user_key.cc',
            'win/lib/user_key.h',
            'win/lib/vista_winnt.h',
          ],
          'dependencies': [
            '../base/base.gyp:base',
          ],
        },
        {
          'target_name': 'rlz',
          'type': 'shared_library',
          'include_dirs': [],
          'sources': [
            'win/dll/dll_main.cc',
            'win/dll/exports.cc',
          ],
          'dependencies': [
            ':rlz_lib',
            '../third_party/zlib/zlib.gyp:zlib',
          ],
        },
        {
          'target_name': 'rlz_unittests',
          'type': 'executable',
          'include_dirs': [],
          'sources': [
            'win/lib/crc32_unittest.cc',
            'win/lib/crc8_unittest.cc',
            'win/lib/financial_ping_test.cc',
            'win/lib/lib_values_unittest.cc',
            'win/lib/machine_deal_test.cc',
            'win/lib/rlz_lib_test.cc',
            'win/lib/string_utils_unittest.cc',
            'win/test/rlz_test_helpers.cc',
            'win/test/rlz_test_helpers.h',
            'win/test/rlz_unittest_main.cc',
          ],
          'dependencies': [
            ':rlz_lib',
            '../base/base.gyp:base',
            '../testing/gmock.gyp:gmock',
            '../testing/gtest.gyp:gtest',
            '../third_party/zlib/zlib.gyp:zlib',
          ],
        },
      ],
    }],
  ],
}
