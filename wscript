#! /usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2006 (ita)

# the following two variables are used by the target "waf dist"
VERSION='0.0.1'
APPNAME='drumscope'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = 'build'

# disable color output
import Logs
Logs.colors_lst['USE'] = False

def set_options(opt):
    # add your custom options here, for example:
    #opt.add_option('--bunny', type='string', help='how many bunnies we have', dest='bunny')
    pass

def configure(conf):
    conf.check_tool('gcc')

    conf.check_cfg(package='glib-2.0', uselib_store='GLIB', atleast_version='2.20.0', args='--cflags --libs', mandatory=True)
    conf.check_cfg(package='gobject-2.0', uselib_store='GOBJECT', atleast_version='2.20.0', args='--cflags --libs', mandatory=True)
    conf.check_cfg(package='clutter-1.0', uselib_store='CLUTTER', atleast_version='1.0.0', mandatory=True, args='--cflags --libs')
    conf.check_cfg(package='gtk+-2.0', uselib_store='GTK', atleast_version='2.16.0', mandatory=True, args='--cflags --libs')
    conf.check_cfg(package='clutter-gtk-0.10', uselib_store='CLUTTER-GTK', atleast_version='0.10.2', mandatory=True, args='--cflags --libs')
    conf.check_cfg(package='alsa', uselib_store='ALSA', atleast_version='1.0.0', mandatory=True, args='--cflags --libs')

    conf.define('VERSION', VERSION)

    conf.write_config_header('config.h')

def build(bld):
    # process subfolders from here
    bld.add_subdirs('src')

