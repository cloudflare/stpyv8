#!/usr/bin/env python

import logging
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from setuptools.command.install import install

from settings import *  # pylint:disable=wildcard-import,unused-wildcard-import

log = logging.getLogger()


def exec_cmd(cmdline, *args, **kwargs):
    msg = kwargs.get("msg")
    cwd = kwargs.get("cwd", ".")
    output = kwargs.get("output")

    if msg:
        print(msg)

    cmdline = " ".join([cmdline] + list(args))

    proc = subprocess.Popen(
        cmdline,  # pylint:disable=consider-using-with
        shell=kwargs.get("shell", True),
        cwd=cwd,
        env=kwargs.get("env"),
        stdout=subprocess.PIPE if output else None,
        stderr=subprocess.PIPE if output else None,
    )

    stdout, stderr = proc.communicate()

    succeeded = proc.returncode == 0

    if not succeeded:
        log.error("%s failed: code = %d", msg or "Execute command", proc.returncode)

        if output:
            log.debug(stderr)

    return succeeded, stdout, stderr if output else succeeded


def install_depot():
    if not os.path.exists(DEPOT_HOME):
        exec_cmd(
            "git clone",
            DEPOT_GIT_URL,
            DEPOT_HOME,
            cwd=os.path.dirname(DEPOT_HOME),
            msg="Cloning depot tools",
        )

        return

    # depot_tools updates itself automatically when running gclient tool
    if os.path.isfile(os.path.join(DEPOT_HOME, "gclient")):
        _, stdout, _ = exec_cmd(
            os.path.join(DEPOT_HOME, "gclient"),  # pylint:disable=unused-variable
            "--version",
            cwd=DEPOT_HOME,
            output=True,
            msg="Found depot tools",
        )


def build_boost():
    url = (
        "https://boostorg.jfrog.io/artifactory/main/release/%s/source/boost_%s.tar.gz"
        % (BOOST_VERSION, BOOST_VERSION.replace(".", "_"))
    )
    if not os.path.isdir(BOOST_HOME):
        os.makedirs(BOOST_HOME)
    bootstrap_path = os.path.join(
        BOOST_HOME, "boost_%s" % BOOST_VERSION.replace(".", "_"), "bootstrap.sh"
    )
    if not os.path.isfile(bootstrap_path):
        print(f"Checkout boost from {url}")
        save_path = tempfile.mkstemp(".tar.gz")[1]
        with open(save_path, "wb") as fp:
            rsp = urllib.request.urlopen(url)
            fp.write(rsp.read())
        tar = tarfile.open(save_path, "r:gz")
        tar.extractall(BOOST_HOME)

    args = ""
    if sys.platform == "darwin":
        args += " compiler.balcklist clang --with-toolset=clang"
    exec_cmd("./bootstrap.sh" + args, cwd=os.path.dirname(bootstrap_path), msg="Run boost bootstrap")
    cxxflags = f"-fPIC $(python{sys.version_info[0]}.{sys.version_info[1]}-config --includes)"
    if sys.platform == "darwin":
        cxxflags += " -stdlib=libc++"
    exec_cmd(
        f'./b2 --with-system --with-iostreams --with-python cxxflags="{cxxflags}" threading=multi',
        cwd=os.path.dirname(bootstrap_path),
        msg="Build boost static libraries",
    )


def checkout_v8():
    if not os.path.exists(V8_HOME):
        exec_cmd(
            os.path.join(DEPOT_HOME, "fetch"),
            "v8",
            cwd=os.path.dirname(V8_HOME),
            msg="Fetching Google V8 code",
        )

    exec_cmd(
        "git fetch --tags", cwd=V8_HOME, msg="Fetching the release tag information"
    )

    exec_cmd(
        "git checkout", V8_GIT_TAG, cwd=V8_HOME, msg=f"Checkout Google V8 v{V8_GIT_TAG}"
    )

    exec_cmd(
        os.path.join(DEPOT_HOME, "gclient"),
        "sync",
        "-D",
        cwd=os.path.dirname(V8_HOME),
        msg="Syncing Google V8 code",
    )

    # On Linux, install additional dependencies, per
    # https://v8.dev/docs/build step 4
    if (
        sys.platform
        in (
            "linux",
            "linux2",
        )
        and v8_deps_linux
    ):
        exec_cmd(
            "./v8/build/install-build-deps.sh",
            cwd=os.path.dirname(V8_HOME),
            msg="Installing additional linux dependencies",
        )


def build_v8():
    exec_cmd(
        os.path.join(DEPOT_HOME, "gn"),
        f"gen out.gn/x64.release.sample --args='{GN_ARGS}'",
        cwd=V8_HOME,
        msg=f"Generate build scripts for V8 (v{V8_GIT_TAG})",
    )

    exec_cmd(
        os.path.join(DEPOT_HOME, "ninja"),
        "-C out.gn/x64.release.sample v8_monolith",
        cwd=V8_HOME,
        msg="Build V8 with ninja",
    )


def clean_stpyv8():
    build_folder = os.path.join(STPYV8_HOME, "build")

    if os.path.exists(os.path.join(build_folder)):
        shutil.rmtree(build_folder)


def prepare_v8():
    try:
        install_depot()
        checkout_v8()
        build_v8()
        # clean_stpyv8()
    except Exception as e:  # pylint:disable=broad-except
        log.error("Fail to checkout and build v8, %s", str(e))


class stpyv8_build_boost(build_ext):
    def run(self):
        build_boost()


class stpyv8_build(build_ext):
    def run(self):
        V8_GIT_TAG = (
            V8_GIT_TAG_STABLE  # pylint:disable=redefined-outer-name,unused-variable
        )
        build_boost()
        if sys.platform == "darwin":
            lib_root = os.path.join(BOOST_ROOT, "stage", "lib")
            for it in os.listdir(lib_root):
                if it.endswith(".dylib"):
                    print(f"Remove file {it}")
                    os.remove(os.path.join(lib_root, it))
        prepare_v8()
        build_ext.run(self)


class stpyv8_develop(build_ext):
    def run(self):
        V8_GIT_TAG = (
            V8_GIT_TAG_MASTER  # pylint:disable=redefined-outer-name,unused-variable
        )
        prepare_v8()
        build_ext.run(self)


class stpyv8_install_v8(build_ext):
    def run(self):
        V8_GIT_TAG = (
            V8_GIT_TAG_MASTER  # pylint:disable=redefined-outer-name,unused-variable
        )
        prepare_v8()


class stpyv8_build_no_v8(build_ext):
    def run(self):
        clean_stpyv8()
        build_ext.run(self)


class stpyv8_install(install):
    def run(self):
        self.skip_build = True  # pylint:disable=attribute-defined-outside-init

        if icu_data_folder:
            os.makedirs(icu_data_folder, exist_ok=True)
            shutil.copy(
                os.path.join(V8_HOME, "out.gn/x64.release.sample/icudtl.dat"),
                icu_data_folder,
            )

        install.run(self)


stpyv8 = Extension(
    name="_STPyV8",
    sources=[os.path.join("src", source) for source in source_files],
    define_macros=macros,
    include_dirs=include_dirs,
    library_dirs=library_dirs,
    libraries=libraries,
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
)

setup(
    name="st-pyv8",
    version=STPYV8_VERSION,
    description="Python Wrapper for Google V8 Engine",
    platforms="x86",
    author="Philip Syme, Angelo Dell'Aera, drunkdream",
    url="https://github.com/drunkdream/stpyv8",
    license="Apache License 2.0",
    py_modules=["STPyV8"],
    ext_modules=[stpyv8],
    classifiers=[
        "Development Status :: 4 - Beta",
        "Environment :: Plugins",
        "Intended Audience :: Developers",
        "Intended Audience :: System Administrators",
        "License :: OSI Approved :: Apache Software License",
        "Natural Language :: English",
        "Programming Language :: C++",
        "Programming Language :: Python",
        "Topic :: Internet",
        "Topic :: Internet :: WWW/HTTP",
        "Topic :: Software Development",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: Utilities",
        "Operating System :: POSIX :: Linux",
        "Operating System :: MacOS :: MacOS X",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
    ],
    cmdclass=dict(
        build_ext=stpyv8_build,
        develop=stpyv8_develop,
        v8=stpyv8_install_v8,
        stpyv8=stpyv8_build_no_v8,
        install=stpyv8_install,
        boost=stpyv8_build_boost,
    ),
)
