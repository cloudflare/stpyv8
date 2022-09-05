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

Installing on Linux/MacOS

```
Shell
$ python3 -m pip install st-pyv8
```

If no wheels are provided for your platform and Python version you are required to build
STPyV8.

# Building

GCC/clang or equivalent and Python3 headers are needed to build the main STPyV8 source
code, as well as boost-python and some other boost dependencies.

## Build Examples

### Linux

```Shell
python_version=3.7
echo Build with python $python_version

dirpath=$(dirname $(realpath $0))

cd docker

sudo docker build -t stpyv8-$python_version --build-arg ARG_PYTHON_VERSION=$python_version .

cd ../

sudo docker run -it -v $dirpath:/stpyv8 --entrypoint /bin/bash stpyv8-$python_version -c 'cd /stpyv8 && python setup.py bdist_wheel --plat-name=manylinux1_x86_64'
```

### MacOS

Building on MacOS requires full [XCode](https://developer.apple.com/xcode/) (not just the
command line tools) to compile Google V8. The command line tools bundled with XCode are
required (rather than the stand-alone command line tools, sometimes requiring
[drastic measures](https://bugs.chromium.org/p/chromium/issues/detail?id=729990#c1) .)

```Shell
$ python3 setup.py bdist_wheel --plat-name macosx_10_10_x86_64
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
