#!/usr/bin/env python

import os
import subprocess
import logging

from distutils.command.build import build
from distutils.core import setup, Extension

from settings import *

log = logging.getLogger()


def exec_cmd(cmdline, *args, **kwargs):
    msg    = kwargs.get('msg')
    cwd    = kwargs.get('cwd', '.')
    output = kwargs.get('output')

    if msg:
        print(msg)

    cmdline = ' '.join([cmdline] + list(args))

    proc = subprocess.Popen(cmdline,
                            shell  = kwargs.get('shell', True),
                            cwd    = cwd,
                            env    = kwargs.get('env'),
                            stdout = subprocess.PIPE if output else None,
                            stderr = subprocess.PIPE if output else None)

    stdout, stderr = proc.communicate()

    succeeded = proc.returncode == 0

    if not succeeded:
        log.error("%s failed: code = %d", msg or "Execute command", proc.returncode)

        if output:
            log.debug(stderr)

    return succeeded, stdout, stderr if output else succeeded


def install_depot():
    if not os.path.exists(DEPOT_HOME):
        exec_cmd("git clone",
                 DEPOT_GIT_URL,
                 DEPOT_HOME,
                 cwd = os.path.dirname(DEPOT_HOME),
                 msg = "Cloning depot tools")

        return

    if os.path.isfile(os.path.join(DEPOT_HOME, 'gclient')):
        _, stdout, _ = exec_cmd(os.path.join(DEPOT_HOME, 'gclient'),
                                "--version",
                                cwd    = DEPOT_HOME,
                                output = True,
                                msg    = "Found depot tools with {}".format(stdout.strip().decode()))

    if os.path.isdir(os.path.join(DEPOT_HOME, '.git')):
        exec_cmd("git pull",
                 DEPOT_HOME,
                 cwd = DEPOT_HOME,
                 msg = "Updating depot tools")


def sync_v8():
    if not os.path.exists(V8_HOME):
        exec_cmd(os.path.join(DEPOT_HOME, 'fetch'), 'v8',
                 cwd = os.path.dirname(V8_HOME),
                 msg = "Fetching Google V8 code")
    elif os.path.exists(os.path.join(SOIRV8_HOME, '.gclient')):
        exec_cmd(os.path.join(DEPOT_HOME, 'gclient'), 'sync',
                cwd = os.path.dirname(V8_HOME),
                msg = "Syncing Google V8 code")

def checkout_v8():
    exec_cmd('git fetch --tags',
             cwd = V8_HOME,
             msg = "Fetching the release tag information")

    exec_cmd('git checkout', V8_GIT_TAG,
             cwd = V8_HOME,
             msg = "Checkout Google V8 v{}".format(V8_GIT_TAG))

    exec_cmd(os.path.join(DEPOT_HOME, 'gclient'), 'sync',
             cwd = os.path.dirname(V8_HOME),
             msg = "Syncing Google V8 code")


def build_v8():
    exec_cmd(os.path.join(DEPOT_HOME, 'gn'),
             "gen out.gn/x64.release.sample --args='{}'".format(GN_ARGS),
             cwd = V8_HOME,
             msg = "Generate build scripts for V8 (v{})".format(V8_GIT_TAG))

    exec_cmd(os.path.join(DEPOT_HOME, 'ninja'),
             "-C out.gn/x64.release.sample v8_monolith",
             cwd = V8_HOME,
             msg = "Build V8 with ninja")


def prepare_v8():
    try:
        install_depot()
        sync_v8()
        checkout_v8()
        build_v8()
    except Exception as e:
        log.error("Fail to checkout and build v8, %s", str(e))


class soirv8_build(build):
    def run(self):
        V8_GIT_TAG = V8_GIT_TAG_STABLE
        prepare_v8()
        build.run(self)

class soirv8_develop(build):
    def run(self):
        V8_GIT_TAG = V8_GIT_TAG_MASTER
        prepare_v8()
        build.run(self)

class soirv8_install_v8(build):
    def run(self):
        V8_GIT_TAG = V8_GIT_TAG_MASTER
        prepare_v8()

class soirv8_build_no_v8(build):
    def run(self):
        build.run(self)


soirv8 = Extension(name               = "_SoirV8",
                   sources            = [os.path.join("src", source) for source in source_files],
                   define_macros      = macros,
                   include_dirs       = include_dirs,
                   library_dirs       = library_dirs,
                   libraries          = libraries,
                   extra_compile_args = extra_compile_args,
                   extra_link_args    = extra_link_args,
                   )

setup(name         = 'soirv8',
      version      = '0.1',
      description  = 'Python Wrapper for Google V8 Engine',
      platforms    = 'x86',
      author       = '',
      author_email = '',
      url          = '',
      download_url = '',
      license      = '',
      py_modules   = ['SoirV8'],
      ext_modules  = [soirv8],
      classifiers  = [
        'Development Status :: 3 - Alpha',
        'Environment :: Plugins',
        'Intended Audience :: Developers',
        'Intended Audience :: System Administrators',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
        'Programming Language :: C++',
        'Programming Language :: Python',
        'Topic :: Internet',
        'Topic :: Internet :: WWW/HTTP',
        'Topic :: Software Development',
        'Topic :: Software Development :: Libraries :: Python Modules',
        'Topic :: Utilities',
      ],
      cmdclass = dict(
          build   = soirv8_build,
          develop = soirv8_develop,
          v8      = soirv8_install_v8,
          soirv8  = soirv8_build_no_v8),
)
