# Copyright 2011 Google Inc. All Rights Reserved.
# Author: siggi@google.com (Sigurdur Asgeirsson)
# Modified by mscdex@mscdex.net (Brian White) to remove Chromium-specific bits

{
  'target_defaults': {
    'include_dirs': [
      '.',
    ],
    'defines': [
      'LINK_SIZE=2',
      'PCRE_STATIC',
      'HAVE_CONFIG_H',
      '_CRT_SECURE_NO_WARNINGS',
    ],
  },
  'targets': [
    {
      'target_name': 'pcre_lib',
      'type': 'static_library',
      'sources': [
        # C sources
        'pcre_compile.c',
        'pcre_config.c',
        'pcre_dfa_exec.c',
        'pcre_exec.c',
        'pcre_fullinfo.c',
        'pcre_get.c',
        'pcre_globals.c',
        #'pcre_info.c',
        'pcre_internal.h',
        'pcre_maketables.c',
        'pcre_newline.c',
        'pcre_ord2utf8.c',
        'pcre_refcount.c',
        'pcre_scanner.h',
        'pcre_study.c',
        'pcre_tables.c',
        #'pcre_try_flipped.c',
        'pcre_ucd.c',
        'pcre_valid_utf8.c',
        'pcre_version.c',
        'pcre_xclass.c',
        'pcrecpp.h',
        'pcrecpp_internal.h',
        'pcreposix.c',
        'pcreposix.h',
        'ucp.h',
        #'<(SHARED_INTERMEDIATE_DIR)/pcre_chartables.c',
        'pcre_chartables.c', 
        # C plusplus sourcews
        'pcrecpp.cc',
        'pcre_scanner.cc',
        #'pcre_stringpiece.cc',
      ],
      'msvs_settings': {
        'VCCLCompilerTool': {
          'AdditionalOptions': ['/wd4018', '/wd4996'],
        },
      },
      'all_dependent_settings': {
        'defines': [
          'LINK_SIZE=2',
          'PCRE_STATIC',
        ],
        'include_dirs': [
          '.',
        ],
      },
    },
  ]
}