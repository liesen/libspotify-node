srcdir = "src"
blddir = 'build'
VERSION = '0.0.1'

import platform
PLATFORM_IS_DARWIN = platform.platform().find('Darwin') == 0

# OS X dylib linker fix
from TaskGen import feature, after
@feature('cshlib')
@after('apply_obj_vars', 'apply_link')
def kill_flag(self):
  fl = self.link_task.env.LINKFLAGS
  if '-bundle' in fl and '-dynamiclib' in fl:
     fl.remove('-bundle')
     self.link_task.env.LINKFLAGS = fl

def set_options(opts):
  opts.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')
  if PLATFORM_IS_DARWIN:
    conf.env.append_value('LINKFLAGS', ['-framework', 'libspotify', '-dynamiclib'])

def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.target = 'binding'
  obj.source = bld.path.ant_glob('src/*.cc')
  if not PLATFORM_IS_DARWIN:
    obj.lib = 'libspotify'

import Options
from os.path import exists
from os import unlink
from shutil import copy2 as copy

def shutdown():
  # HACK to get binding.node out of build directory
  if Options.commands['clean']:
    if exists('spotify/binding.node'): unlink('spotify/binding.node')
  else:
    if exists('build/default/binding.node'):
      copy('build/default/binding.node', 'spotify/binding.node')
