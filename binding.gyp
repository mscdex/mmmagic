{
  'targets': [
    {
      'target_name': 'magic',
      'sources': [
        'src/binding.cc',
      ],
      'include_dirs': [
        'deps/libmagic/src',
        "<!(node -e \"require('nan')\")"
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
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'MACOSX_DEPLOYMENT_TARGET': '10.7',
            'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
            'CLANG_CXX_LANGUAGE_STANDARD': 'gnu++1y',  # -std=gnu++1y
            'CLANG_CXX_LIBRARY': 'libc++',
          }
        }],
      ],
    },
  ],
}
