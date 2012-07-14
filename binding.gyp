{
  'targets': [
    {
      'target_name': 'magic',
      'sources': [
        'magic.cc',
      ],
      'include_dirs': [
        'deps/libmagic/src',
      ],
      'cflags': [ '-O3' ],
      'dependencies': [
        'deps/libmagic/binding.gyp:libmagic_lib',
      ],
    },
  ],
}