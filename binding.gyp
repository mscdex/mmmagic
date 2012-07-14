{
  'targets': [
    {
      'target_name': 'magic',
      'sources': [
        'magic.cc',
      ],
      'cflags': [ '-O3' ],
      'dependencies': [
        'deps/libmagic/binding.gyp:libmagic_lib',
      ],
    },
  ],
}