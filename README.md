# STPyV8

STPyV8 allows interop between Python 3 and JavaScript running Google's V8 engine.  Use STPyV8 to embed JavaScript code directly into your Python project, or to call Python code from JavaScript.

STPyV8 is a fork of the original [PyV8](https://code.google.com/archive/p/pyv8/) project, with code changed to work with the latest V8 engine.  STPyV8 links with Google V8 built as a static library. Currently the library builds on Linux and MacOS, with Windows planned for the future.

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

# Building

GCC/clang or equivalent and Python3 headers are needed to build the main STPyV8 source code, as well as boost-python and some other boost dependencies. For a short while, Python 2.7 is still needed by Google's toolchain to build a local library version of V8.

A Python3 virtual environment is recommended.  (Google's build tools will establish their own Python2 virtual environment during the compilation of V8, but this can be ignored.)

## Build Examples

### Ubuntu
Building on Ubuntu 18.04, 19.10, and Debian distros:

```Shell
$ sudo apt install python python3 python3-venv python3-dev build-essential libboost-dev libboost-system-dev libboost-python-dev
$ # This step will take some time, install other packages (with sudo), to build V8 as a static library
$ python2 setup.py v8 
$ python3 -m venv env
$ source env/bin/activate
$ python setup.py stpyv8
$ python setup.py install
```

Building on Ubuntu 16.04 requires an external PPA addition for python3.  Building on other Linux distributions requires appropriate use of their package managers for these external dependencies, and some gymnastics for the V8 build dependencies.

### MacOS

Building on MacOS requires full [XCode](https://developer.apple.com/xcode/) (not just the command line tools) to compile V8.  The command line tools bundled with XCode are required (rather than the stand-alone command line tools, sometime requiring [drastic measures](https://bugs.chromium.org/p/chromium/issues/detail?id=729990#c1).

Using [HomeBrew](https://brew.sh) makes the boost-python and related dependencies easier for STPyV8:

```Shell
$ brew install boost-python3
$ # This step will take some time, to build V8 as a static library
$ python2 setup.py v8 
$ python3 -m venv env
$ source env/bin/activate
$ python setup.py stpyv8
$ python setup.py install
```

More detailed build instructions are in the [docs](docs/source/build.rst) folder.

# How does this work?
STPyV8 is a Python [C++ Extension Module](https://docs.python.org/3/c-api/index.html) that links to an [embedded V8](https://v8.dev/docs/embed) library.  Since PyV8 used the [Boost.Python C++](https://www.boost.org/doc/libs/1_70_0/libs/python/doc/html/index.html) library (as wells as some others) we kept it, but future work may include just using the C API exposed by Python and eliminating boost.  Think of this as an Oreo cookie - Python and V8 crackers with C++ icing in the middle gluing them together.

## Is STPyV8 fast?
STPyV8 needs to translate Python arguments (and JavaScript arguments) back and forth between function and method calls in the two languages. It does the minimum amount of work using native code, but if you are interested in the height of performance, make your interface between Python and JavaScript "chunky" ... i.e., make the minimum number of transitions between the two.

# What can I use this for?
We use STPyV8 to simulate a [browser](https://github.com/buffer/thug), and then execute sketchy JavaScript in an instrumented container.  Other kinds of JavaScript sand-boxing (simulating and monitoring the external world to JavaScript code) are a natural fit.  Other uses include squeezing in some "bare-metal" [hyper-optimized](https://nodesource.com/blog/why-the-new-v8-is-so-damn-fast/) (by V8) JavaScript into your Python projects, say as a stepping-stone to a full [node.js](https://nodejs.org/) port.
