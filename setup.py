#!/usr/bin/env python

import sys
import os
import subprocess
import shutil
import logging

from distutils.command.build import build
from distutils.command.install import install

try:
    from setuptools import setup, Extension
except ImportError:
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

    # depot_tools updates itself automatically when running gclient tool
    if os.path.isfile(os.path.join(DEPOT_HOME, 'gclient')):
        _, stdout, _ = exec_cmd(os.path.join(DEPOT_HOME, 'gclient'),
                                "--version",
                                cwd    = DEPOT_HOME,
                                output = True,
                                msg    = "Found depot tools")


def checkout_v8():
    if not os.path.exists(V8_HOME):
        exec_cmd(os.path.join(DEPOT_HOME, 'fetch'),
                 'v8',
                 cwd = os.path.dirname(V8_HOME),
                 msg = "Fetching Google V8 code")

    exec_cmd('git fetch --tags',
             cwd = V8_HOME,
             msg = "Fetching the release tag information")

    exec_cmd('git checkout',
             V8_GIT_TAG,
             cwd = V8_HOME,
             msg = "Checkout Google V8 v{}".format(V8_GIT_TAG))

    exec_cmd(os.path.join(DEPOT_HOME, 'gclient'),
             'sync',
             '-D',
             cwd = os.path.dirname(V8_HOME),
             msg = "Syncing Google V8 code")

    # On Linux, install additional dependencies, per
    # https://v8.dev/docs/build step 4
    if sys.platform in ("linux", "linux2", ) and v8_deps_linux:
        exec_cmd('./v8/build/install-build-deps.sh',
                 cwd = os.path.dirname(V8_HOME),
                 msg = "Installing additional linux dependencies")

def build_v8():
    exec_cmd(os.path.join(DEPOT_HOME, 'gn'),
             "gen out.gn/x64.release.sample --args='{}'".format(GN_ARGS),
             cwd = V8_HOME,
             msg = "Generate build scripts for V8 (v{})".format(V8_GIT_TAG))

    exec_cmd(os.path.join(DEPOT_HOME, 'ninja'),
             "-C out.gn/x64.release.sample v8_monolith",
             cwd = V8_HOME,
             msg = "Build V8 with ninja")


def clean_stpyv8():
    build_folder = os.path.join(STPYV8_HOME, 'build')

    if os.path.exists(os.path.join(build_folder)):
        shutil.rmtree(build_folder)


def prepare_v8():
    try:
        install_depot()
        checkout_v8()
        build_v8()
        clean_stpyv8()
    except Exception as e:
        log.error("Fail to checkout and build v8, %s", str(e))


class stpyv8_build(build):
    def run(self):
        V8_GIT_TAG = V8_GIT_TAG_STABLE
        prepare_v8()
        build.run(self)


class stpyv8_develop(build):
    def run(self):
        V8_GIT_TAG = V8_GIT_TAG_MASTER
        prepare_v8()
        build.run(self)


class stpyv8_install_v8(build):
    def run(self):
        V8_GIT_TAG = V8_GIT_TAG_MASTER
        prepare_v8()


class stpyv8_build_no_v8(build):
    def run(self):
        clean_stpyv8()
        build.run(self)


class stpyv8_install(install):
    def run(self):
        self.skip_build = True
        install.run(self)


stpyv8 = Extension(name               = "_STPyV8",
                   sources            = [os.path.join("src", source) for source in source_files],
                   define_macros      = macros,
                   include_dirs       = include_dirs,
                   library_dirs       = library_dirs,
                   libraries          = libraries,
                   extra_compile_args = extra_compile_args,
                   extra_link_args    = extra_link_args,
                   )

setup(name         = "stpyv8",
      version      = STPYV8_VERSION,
      description  = "Python Wrapper for Google V8 Engine",
      platforms    = "x86",
      author       = "Philip Syme, Angelo Dell'Aera",
      url          = "https://github.com/area1/stpyv8",
      license      = "Apache License 2.0",
      py_modules   = ["STPyV8"],
      ext_modules  = [stpyv8],
      classifiers  = [
        "Development Status :: 4 - Beta",
        "Environment :: Plugins",
        "Intended Audience :: Developers",
        "Intended Audience :: System Administrators",
        "License :: OSI Approved :: Apache Software License",
        "Natural Language :: English",
        "Operating System :: POSIX",
        "Programming Language :: C++",
        "Programming Language :: Python",
        "Topic :: Internet",
        "Topic :: Internet :: WWW/HTTP",
        "Topic :: Software Development",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: Utilities",
      ],
      cmdclass = dict(
          build   = stpyv8_build,
          develop = stpyv8_develop,
          v8      = stpyv8_install_v8,
          stpyv8  = stpyv8_build_no_v8,
          install = stpyv8_install),
)
