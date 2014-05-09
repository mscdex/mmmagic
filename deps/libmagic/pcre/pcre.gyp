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
      'target_name': 'libpcre',
      'type': 'static_library',
      'sources': [
        # C sources
        'pcre_byte_order.c',
        'pcre_chartables.c',
        'pcre_compile.c',
        'pcre_config.c',
        'pcre_dfa_exec.c',
        'pcre_exec.c',
        'pcre_fullinfo.c',
        'pcre_get.c',
        'pcre_globals.c',
        'pcre_jit_compile.c',
        'pcre_maketables.c',
        'pcre_newline.c',
        'pcre_ord2utf8.c',
        'pcre_refcount.c',
        'pcre_string_utils.c',
        'pcre_study.c',
        'pcre_tables.c',
        'pcre_ucd.c',
        'pcre_valid_utf8.c',
        'pcre_version.c',
        'pcre_xclass.c',
        'pcreposix.c',
      ],
      'cflags!': [ '-O2' ],
      'cflags+': [ '-O3' ],
      'cflags_cc!': [ '-O2' ],
      'cflags_cc+': [ '-O3' ],
      'cflags_c!': [ '-O2' ],
      'cflags_c+': [ '-O3' ],
      'msvs_settings': {
        'VCCLCompilerTool': {
          'AdditionalOptions': ['/wd4018', '/wd4996'],
        },
      },
      'conditions': [
        [ 'OS=="win"', {
          'include_dirs': [ 'config/win' ],
        }],
        [ 'OS=="linux"', {
          'include_dirs': [ 'config/linux' ],
        }],
        [ 'OS=="mac"', {
          'include_dirs': [ 'config/mac' ],
        }],
        [ 'OS=="freebsd"', {
          'include_dirs': [ 'config/freebsd' ],
        }],
        [ 'OS=="openbsd"', {
          'include_dirs': [ 'config/openbsd' ],
        }],
        [ 'OS=="solaris"', {
          'include_dirs': [ 'config/sunos' ],
        }],
      ],
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