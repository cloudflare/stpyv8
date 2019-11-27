.. SoirV8 documentation master file

.. _index:
.. py:module:: SoirV8

Welcome to SoirV8's documentation!
==================================

SoirV8 is a Python wrapper for the Google V8 engine [#f1]_ acting as a bridge between the Python and JavaScript
objects and supporting the embedding of the Google V8 engine in a Python script.

Creating and entering a context to evaluate Javascript code can be done in a few lines of code.

.. code-block:: python

   >>> ctxt = JSContext()   # create a context with an implicit global object
   >>> ctxt.enter()         # enter the context (Python with statement is also supported)
   >>> ctxt.eval("1+2")     # evalute the Javascript expression and return a native Python integer
   3

You also could invoke Javascript functions from Python, or viceversa.

.. code-block:: python

    >>> class Global(JSClass):           # define a compatible Javascript class
    ...   def hello(self):               # define a method
    ...     print("Hello World")
    ...
    >>> ctxt2 = JSContext(Global())      # create another context with the global object
    >>> ctxt2.enter()
    >>> ctxt2.eval("hello()")            # call the global object from Javascript
    Hello World                          

.. toctree::
   :maxdepth: 2

   intro
   build
   samples
   api/api
   faq

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

.. rubric:: Footnotes

.. [#f1] `Google V8 <https://v8.dev/>`

   V8 is Google’s open source high-performance JavaScript and WebAssembly engine, written in C++.
   
   It is used in Chrome and in Node.js, among others. It implements ECMAScript and WebAssembly, and runs
   on Windows 7 or later, macOS 10.12+, and Linux systems that use x64, IA-32, ARM, or MIPS processors.
   
   V8 can run standalone, or can be embedded into any C++ application.
