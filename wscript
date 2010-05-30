blddir = 'build'
VERSION = '0.0.1'

def set_options(opts):
    opts.tool_options('compiler_cxx')

def configure(conf):
    conf.check_tool('compiler_cxx')
    conf.check_tool('node_addon')

def build(bld):
    obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
    obj.target = 'spotify'
    obj.source = bld.path.ant_glob('src/*.cc') 
    obj.uselib = 'libspotify'

