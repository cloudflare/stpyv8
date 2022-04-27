# STPyV8

STPyV8 allows interoperability between Python 3 and JavaScript running Google V8 engine.
Use STPyV8 to embed JavaScript code directly into your Python project, or to call Python
code from JavaScript.

STPyV8 is a fork of the original [PyV8](https://code.google.com/archive/p/pyv8/) project,
with code changed to work with the latest Google V8 engine and Python 3. STPyV8 links with
Google V8 built as a static library.

Currently the library builds on Linux and MacOS, with Windows planned for the future.

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

The easiest way to install STPyV8 is to use one of the Python wheels provided at
[Releases](https://github.com/cloudflare/stpyv8/releases). The wheels are automatically
generated using Github Actions and multiple platforms and Python versions are already
supported, with others planned for the future.

Be aware that boost-python and some other boost dependencies are needed (see later for
details). Most Linux distributions and MacOS provide easy to install Boost packages and
this is the suggested way to install the library.

Each zip file contains the ICU data file icudtl.dat and the wheel itself. First of all you
should copy icudtl.data to the STPyV8 ICU data folder (Linux: /usr/share/stpyv8, MacOS:
/Library/Application Support/STPyV8/) and then install/upgrade STPyV8 using pip.

Installing on MacOS

```
Shell
$ unzip stpyv8-macos-10.15-python-3.9.zip
Archive:  stpyv8-macos-10.15-python-3.9.zip
  inflating: stpyv8-macos-10.15-3.9/icudtl.dat
  inflating: stpyv8-macos-10.15-3.9/stpyv8-9.9.115.8-cp39-cp39-macosx_10_15_x86_64.whl
$ cd stpyv8-macos-10.15-3.9
$ sudo mv icudtl.dat /Library/Application\ Support/STPyV8
$ sudo pip install --upgrade stpyv8-9.9.115.8-cp39-cp39-macosx_10_15_x86_64.whl
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

### Ubuntu
Building on Ubuntu 18.04, 19.10, 20.04 and Debian distros:

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
