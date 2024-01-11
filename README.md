# STPyV8

STPyV8 allows interoperability between Python 3 and JavaScript running Google V8 engine.
Use STPyV8 to embed JavaScript code directly into your Python project, or to call Python
code from JavaScript.

STPyV8 is a fork of the original [PyV8](https://code.google.com/archive/p/pyv8/) project,
with code changed to work with the latest Google V8 engine and Python 3. STPyV8 links with
Google V8 built as a static library.

Currently the library builds on Linux, MacOS and Windows.

# Usage Examples

Wrapping a JavaScript function in a Python function:

```Python
# simple.py
import STPyV8

with STPyV8.JSContext() as ctxt:
  upcase = ctxt.eval("""
    ( (lowerString) => {
        return lowerString.toUpperCase();
    })
  """)
  print(upcase("hello world!"))
```

```Shell
$ python simple.py
HELLO WORLD!
```

## Using Python in V8

STPyV8 allows you to use Python functions, classes, and objects from within V8.

Exporting a Python class into V8 and using it from JavaScript:

```Python
# meaning.py
import STPyV8

class MyClass(STPyV8.JSClass):
  def reallyComplexFunction(self, addme):
    return 10 * 3 + addme

my_class = MyClass()

with STPyV8.JSContext(my_class) as ctxt:
  meaning = ctxt.eval("this.reallyComplexFunction(2) + 10;")
  print("The meaning of life: " + str(meaning))
```

```Shell
$ python meaning.py
The meaning of life: 42
```

## Using JavaScript in Python

STPyV8 allows you to use JavaScript functions, classes, object from Python.

Calling methods on a JavaScript class from Python code:

```Python
# circle.py
import STPyV8

with STPyV8.JSContext() as ctxt:
  ctxt.eval("""
    class Circle {
      constructor(radius) {
        this.radius = radius;
      }
      get area() {
        return this.calcArea()
      }
      calcArea() {
        return 3.14 * this.radius * this.radius;
      }
  }
  """)
  circle = ctxt.eval("new Circle(10)")
  print("Area of the circle: " + str(circle.area))
```

```Shell
$ python cicle.py
Area of the circle: 314
```

Find more in the [tests](tests) directory.

# Installing

STPyV8 is avaliable on PyPI (starting from release v12.0.267.16) and officially supports
Python 3.9+

```Shell
$ pip install stpyv8
```

Be aware that, starting from STPyV8 v12.0.267.14, installing boost-python and some other
boost dependencies is not required anymore while it is still required if you're installing
older versions (see later for details). Most Linux distributions and MacOS provide easy to
install Boost packages and this is the suggested way to install the library in case you
need it.

If you are planning to install a version older than v12.0.267.16 you should use one of
the Python wheels provided at [Releases](https://github.com/cloudflare/stpyv8/releases).
The wheels are automatically generated using Github Actions and multiple platforms and
Python versions are supported. In such case, you need to download the zip file for the
proper platform and Python version. Each zip file contains the ICU data file icudtl.dat
and the wheel itself. First of all you should copy icudtl.data to the STPyV8 ICU data
folder (Linux: /usr/share/stpyv8, MacOS: /Library/Application Support/STPyV8/) and then
install/upgrade STPyV8 using pip.

Installing on MacOS

```Shell
$ unzip stpyv8-macos-10.15-python-3.9.zip
Archive:  stpyv8-macos-10.15-python-3.9.zip
  inflating: stpyv8-macos-10.15-3.9/icudtl.dat
  inflating: stpyv8-macos-10.15-3.9/stpyv8-9.9.115.8-cp39-cp39-macosx_10_15_x86_64.whl
$ cd stpyv8-macos-10.15-3.9
$ sudo mv icudtl.dat /Library/Application\ Support/STPyV8
$ pip install --upgrade stpyv8-9.9.115.8-cp39-cp39-macosx_10_15_x86_64.whl
Processing ./stpyv8-9.9.115.8-cp39-cp39-macosx_10_15_x86_64.whl
Installing collected packages: stpyv8
Successfully installed stpyv8-9.9.115.8
```

If no wheels are provided for your platform and Python version you are required to build
STPyV8.

# Building

GCC/clang or equivalent and Python3 headers are needed to build the main STPyV8 source
code, as well as boost-python and some other boost dependencies.

## Build Examples

### Ubuntu/Debian
Building on Ubuntu and Debian distros:

```Shell
$ sudo apt install python3 python3-dev build-essential libboost-dev libboost-system-dev libboost-python-dev libboost-iostreams-dev
$ python setup.py build
$ sudo python setup.py install
```

Building on other Linux distributions requires appropriate use of their package managers
for these external dependencies, and some gymnastics for the V8 build dependencies.

### MacOS

Building on MacOS requires full [XCode](https://developer.apple.com/xcode/) (not just the
command line tools) to compile Google V8. The command line tools bundled with XCode are
required (rather than the stand-alone command line tools, sometimes requiring
[drastic measures](https://bugs.chromium.org/p/chromium/issues/detail?id=729990#c1) .)

Using [HomeBrew](https://brew.sh) makes the boost-python and related dependencies easier for
STPyV8:

```Shell
$ brew install boost-python3
$ python setup.py build
$ sudo python setup.py install
```

More detailed build instructions are in the [docs](docs/source/build.rst) folder.

### Windows

Please note that building STPyV8 on Windows may take a while and can be a pretty involved process. You're encouraged to download a precompiled binary from this repository.

Here are the prerequisites for building on Windows:

* MSVC 14.20 (packaged with Visual Studio 2019)
* [Boost](https://boostorg.jfrog.io/ui/repos/tree/General/main/release)

The following environment variables must be set:
* `BOOST_ROOT` - Boost installation directory (e.g. `C:\local\boost_1_83_0`)
* `Python_ROOT_DIR` - Python installation directory (e.g. `C:\python311`) 

#### Boost installation with precompiled binaries

If your Boost installation comes with precompiled binaries you'll have to make sure they can be used to build this project.

The binaries required are statically-linkable LIB files compiled with MSVC 14.20 and may look something like this (in a Boost 1.82 installation):

    boost_python310-vc142-mt-s-x64-1_82.lib

If you were able to locate a similar file for your Python version you might not need to build Boost.

If the LIB file you found is not located in the directory `$env:BOOST_DIR\stage\lib` then you must add its containing directory path to the `LIB` environment variable.

For example, if you installed Boost through an official installer, the LIB file might be in the `lib64-msvc-14.2` directory. In this case: `$env:LIB = "$env:LIB;$env:BOOST_ROOT\lib64-msvc-14.2"`.

If you weren't able to located the correct file, or you encountered linking errors further down the build process, you'll have to build Boost. Here is an example of one such linking error:

    LINK : fatal error LNK1104: cannot open file 'libboost_python310-vc142-mt-s-x32-1_74.lib'

#### Building Boost

To build the Boost.Python component of Boost with Powershell Developer Console:

```powershell
cd $env:BOOST_ROOT
.\bootstrap.bat
```

Before building you must tell Boost which Python version you're building for. To do this, add the following line to the end of `project-config.jam`:

    using python : : "C:\\python311" ;

NOTE: Use the actual path to your Python installation and ensure backslases are escaped. This directory should include `python.exe`, an `include` directory and a `libs` directory.

Back to Powershell:

```powershell
.\b2.exe stage -j 8 link=static runtime-link=static --with-python --with-iostreams --with-date_time --with-thread
```

The boost binaries will be generated to `$env:BOOST_ROOT\stage\lib`.

#### Building STPyV8

Once you've got your Boost binaries you're ready to build STPyV8.

From Powershell, `cd` into the project root:

```powershell
python -m pip install wheel
python setup.py bdist_wheel
```

Once the second command is done (may take quite a while) you'll have a wheel file ready to be installed.

# How does this work?
STPyV8 is a Python [C++ Extension Module](https://docs.python.org/3/c-api/index.html) that
links to an [embedded V8](https://v8.dev/docs/embed) library.  Since PyV8 used the
[Boost.Python C++](https://www.boost.org/doc/libs/1_70_0/libs/python/doc/html/index.html)
library (as wells as some others) we kept it, but future work may include just using the C
API exposed by Python and eliminating boost. Think of this as an Oreo cookie - Python and
Google V8 crackers with C++ icing in the middle gluing them together.

## Is STPyV8 fast?
STPyV8 needs to translate Python arguments (and JavaScript arguments) back and forth between
function and method calls in the two languages. It does the minimum amount of work using native
code, but if you are interested in the height of performance, make your interface between
Python and JavaScript "chunky" ... i.e., make the minimum number of transitions between the two.

# What can I use this for?
We use STPyV8 to simulate a [browser](https://github.com/buffer/thug), and then execute sketchy
JavaScript in an instrumented container. Other kinds of JavaScript sandboxing (simulating and
monitoring the external world to JavaScript code) are a natural fit.
