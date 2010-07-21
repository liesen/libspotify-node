from shutil import copy2 as copy
import Options
import Utils
import os
import os.path as path
import platform


srcdir = "."
blddir = 'build'
VERSION = '0.0.1'

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
  opts.add_option('--debug', action='store_true', default=False,
                 help='build debug version')

def configure(conf):
  import Options
  # todo: add --debug flag so we can set NDEBUG conditionally, omitting asserts.
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')
  if PLATFORM_IS_DARWIN:
    conf.env.append_value('LINKFLAGS', ['-framework', 'libspotify',
                                        '-dynamiclib'])
  conf.env.append_value('LINKFLAGS', [os.getcwdu()+'/src/atomic_queue.o'])
  if Options.options.debug:
    conf.env['CXXFLAGS'] = list(conf.env['CXXFLAGS_DEBUG'])
  else:
    conf.env['CXXFLAGS'] = list(conf.env['CXXFLAGS_RELEASE'])
    conf.env.append_value('CXXFLAGS', ['-DNDEBUG'])

def lint(ctx):
  Utils.exec_command('python cpplint.py --verbose=0 --filter='+
    '-legal/copyright,' +     # in the future
    '-build/header_guard,' +  # not interesting
    '-build/include,' +       # lint is run from outside src
    '-build/namespaces,' +    # we are not building a C++ API
    '-whitespace/comments,' +
    ' src/*.cc' +
    ' $(find src \! -name queue.h -name *.h)')

def build(ctx):
  ctx.add_pre_fun(lint)
  task = ctx.new_task_gen('cxx', 'shlib', 'node_addon')
  task.target = 'binding'
  task.source = ctx.path.ant_glob('src/*.cc')
  if not PLATFORM_IS_DARWIN:
    task.lib = 'libspotify'
  # TODO: fix this ugly hack:
  from subprocess import Popen, PIPE
  Popen(['cc','-c','-o','src/atomic_queue.o','src/atomic_queue.s'],
    stderr=PIPE).communicate()[1]

def test(ctx):
  status = Utils.exec_command('node test/all.js')
  if status != 0:
    raise Utils.WafError('tests failed')

def shutdown():
  # HACK to get binding.node out of build directory
  if Options.commands['clean']:
    if path.exists('spotify/binding.node'): os.unlink('spotify/binding.node')
    if path.exists('src/atomic_queue.o'): os.unlink('src/atomic_queue.o')
  else:
    if path.exists('build/default/binding.node'):
      copy('build/default/binding.node', 'spotify/binding.node')

