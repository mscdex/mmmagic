{
  'targets': [
    {
      'target_name': 'magic',
      'sources': [
        'src/magic.cc',
      ],
      'include_dirs': [
        "<!(node -e \"require('nan')\")",
        'deps/libmagic/src'
      ],
      'cflags!': [ '-O2' ],
      'cflags+': [ '-O3' ],
      'cflags_cc!': [ '-O2' ],
      'cflags_cc+': [ '-O3' ],
      'cflags_c!': [ '-O2' ],
      'cflags_c+': [ '-O3' ],
      'dependencies': [
        'deps/libmagic/libmagic.gyp:libmagic',
      ],
    },
  ],
}
