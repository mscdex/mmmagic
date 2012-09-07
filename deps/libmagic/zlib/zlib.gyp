# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'zlib',
      'type': 'static_library',
      'cflags': [ '-O3' ],
      'sources': [
        'adler32.c',
        'compress.c',
        'crc32.c',
        'crc32.h',
        'deflate.c',
        'deflate.h',
        'gzclose.c',
        'gzguts.h',
        'gzlib.c',
        'gzread.c',
        'gzwrite.c',
        'infback.c',
        'inffast.c',
        'inffast.h',
        'inffixed.h',
        'inflate.c',
        'inflate.h',
        'inftrees.c',
        'inftrees.h',
        'trees.c',
        'trees.h',
        'uncompr.c',
        'zconf.h',
        'zlib.h',
        'zutil.c',
        'zutil.h',
      ],
      'include_dirs': [
        '.',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '.',
        ],
      },
    },
  ],
}
