import Options
from os import unlink, symlink
from os.path import exists, abspath
import os

srcdir = '.'
blddir = 'build'
depdir = abspath(srcdir) + "/deps/libmagic"
VERSION = '0.0.1'

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')

  # configure file/libmagic
  print "Configuring libmagic library ..."
  cflags = ""
  if conf.env['DEST_CPU'] == 'x86_64':
    cflags = "CFLAGS=-fPIC "
  cmd = "cd deps/libmagic && " + cflags + " sh configure --disable-shared"
  if os.system(cmd) != 0:
    conf.fatal("Configuring libmagic failed.")

def build(bld):
  print "Building libmagic library ..."
  cmd = "cd deps/libmagic && make"
  if os.system(cmd) != 0:
    conf.fatal("Building libmagic failed.")
  else:
    obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
    obj.target = 'magic'
    obj.source = 'magic.cc'
    obj.includes = [depdir + '/src']
    obj.cxxflags = ['-O3']
    obj.linkflags = [depdir + '/src/.libs/libmagic.a', '-lz']

def shutdown():
  # HACK to get magic.node out of build directory.
  # better way to do this?
  if Options.commands['clean']:
    if exists('magic.node'): unlink('magic.node')
  else:
    if exists('build/Release/magic.node') and not exists('magic.node'):
      symlink('build/Release/magic.node', 'magic.node')
    if exists('build/default/magic.node') and not exists('magic.node'):
      symlink('build/default/magic.node', 'magic.node')
