#!/usr/bin/env python

import os
from distutils.core import setup, Extension

source_files = ["Exception.cpp", 
                "Context.cpp",
                "Engine.cpp",
                "Wrapper.cpp",
                "Locker.cpp",
                "Utils.cpp",
                "Main.cpp"]

macros = [("BOOST_PYTHON_STATIC_LIB", None)]
third_party_libraries = ["python", "boost", "v8"]

include_dirs = [os.path.join("lib", lib, "inc") for lib in third_party_libraries]
library_dirs = [os.path.join("lib", lib, "lib") for lib in third_party_libraries]

V8_HOME = os.getenv('V8_HOME', os.getenv('HOME'))
include_dirs.append(os.path.join(V8_HOME, 'v8/include'))
library_dirs.append(os.path.join(V8_HOME, 'v8/out.gn/x64.release.sample/obj/'))

libraries = []
extra_compile_args = []
extra_link_args = []
 

if os.name == "nt":
  include_dirs += os.environ["INCLUDE"].split(';')
  library_dirs += os.environ["LIB"].split(';')
  libraries += ["winmm", "ws2_32"]
  extra_compile_args += ["/O2", "/GL", "/MT", "/EHsc", "/Gy", "/Zi"]
  extra_link_args += ["/DLL", "/OPT:REF", "/OPT:ICF", "/MACHINE:X86"]
elif os.name == "posix":
  libraries = ["boost_python", "boost_system", "v8_monolith", "rt"]


soirv8 = Extension(name               = "_SoirV8",
                   sources            = [os.path.join("src", file) for file in source_files],                 
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
      ]
)
