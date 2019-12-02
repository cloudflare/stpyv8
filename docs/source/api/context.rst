.. py:module:: PyV8

.. testsetup:: *

   from PyV8 import *

.. _context:

Javascript Context
==================

.. sidebar:: execution context

    When control is transferred to ECMAScript executable code [#f2]_, control is entering an execution context.

        -- ECMA-262 3rd Chapter 10

According to the ECMAScript [#f1]_ standard, we need enter an execution context before execute any script code.

:py:class:`JSContext` is a sandboxed execution context with its own set of built-in objects and functions.

You could create a :py:class:`JSContext` instance, enter it with :py:meth:`JSContext.enter`, and use it to execute code with :py:meth:`JSContext.eval`. The best practice is to leave the context with :py:meth:`JSContext.leave` if you never need it.

.. doctest::

   >>> ctxt = JSContext()               # create a context with an implicit global object
   >>> ctxt.enter()                     # enter the context (also support with statement)
   >>> ctxt.eval("1+2")                 # evalute the javascript expression and return a native python int
   3
   >>> ctxt.leave()                     # leave the context and release the related resources

.. note::

   To ensure the context could be enter/leave at the right time, we suggest to use the **with** statement (Python 2.5 or later).

    .. testcode::

        with JSContext() as ctxt:
            print ctxt.eval("1+2") # 3

    .. testoutput::
       :hide:

       3

You could also check the current or calling context with the static properties.

==============================  =============================================
Property                        Description
==============================  =============================================
:py:attr:`JSContext.current`    The context that is on the top of the stack.
:py:attr:`JSContext.entered`    The last entered context.
:py:attr:`JSContext.calling`    The context of the calling JavaScript code.
:py:attr:`JSContext.inContext`  Returns true if V8 has a current context.
==============================  =============================================

.. _gobj:

Global Object
-------------

.. sidebar:: global object

    There is a unique *global object* (15.1), which is created before control enters any execution context. Initially the global object has the following properties:

    * Built-in objects such as Math, String, Date, parseInt, etc.
    * Additional host defined properties.

    As control enters execution contexts, and as ECMAScript code is executed, additional properties may be added to the global object and the initial properties may be changed.

        -- ECMA-262 3rd Chapter 10.1.5

The execution context has a global object, which could be access from the Python side with :py:attr:`JSContext.locals`, or access from the Javascript side with the global namespace. The Python and Javascript code could use it to do seamless interoperable logic, PyV8 will automatic do the :ref:`typeconv`, :ref:`funcall` and :ref:`exctrans`.

.. testcode::

    with JSContext() as ctxt:
        ctxt.eval("a = 1")
        print ctxt.locals.a     # 1

        ctxt.locals.a = 2
        print ctxt.eval("a")    # 2

.. testoutput::
   :hide:

   1
   2

If you want to provide more complicated properties and methods to the Javascript code, you could pass a customized global object instance when create the :py:class:`JSContext`.

.. testcode::

    class Global(JSClass):
        version = "1.0"

        def hello(self, name):
            return "Hello " + name

    with JSContext(Global()) as ctxt:
        print ctxt.eval("version")          # 1.0
        print ctxt.eval("hello('World')")   # Hello World
        print ctxt.eval("hello.toString()") # function () { [native code] }

.. testoutput::
   :hide:

   1.0
   Hello World
   function () { [native code] }

.. note::

    If you want your global object more like a real Javascript object, you should inherit from the :py:class:`JSClass` which provides a lot of helper methods such as :py:meth:`JSClass.toString`, :py:meth:`JSClass.watch` etc.

.. _jsext:

Reuseable Extension
-------------------
Besides the customized :ref:`gobj`, there is a more powerful mechanism :py:class:`JSExtension` which could encapsulate the related Javascript or Python script in a reusable module, and define it when create a :py:class:`JSContext` instance.

.. testcode::

    src = 'function hello(name) { return "Hello " + name + " from Javascript"; }'
    ext = JSExtension("hello/javascript", src)

    with JSContext(extensions=['hello/javascript']) as ctxt:
        print ctxt.eval("hello('World')") # Hello World from Javascript

.. testoutput::
   :hide:

   Hello World from Javascript

The extension also could be implement with the Python code, just like the JNI [#f3]_ for the Java.

.. testcode::

    src = "native function hello();"
    ext = JSExtension("hello/python", src, lambda func: lambda name: "Hello " + name + " from Python")

    with JSContext(extensions=['hello/python']) as ctxt:
        print ctxt.eval("hello('World')") # Hello World from Python

.. testoutput::
   :hide:

   Hello World from Python

If your extensions has dependencies extensions, you could define it in :py:meth:`JSExtension.__init__`, and check it later with the  :py:attr:`JSExtension.dependencies` property. The v8 engine will load the extensions one by one base on its dependencies.

JSContext - the execution context.
----------------------------------

.. autoclass:: JSContext
   :members:
   :inherited-members:
   :exclude-members: eval

   JSContext is an execution context.

   .. automethod:: __init__(global=None, extensions=[]) -> JSContext object

      :param object global: the global object
      :param list extensions: the name of extensions
      :rtype: :py:class:`JSContext` instance

   .. automethod:: __init__(ctxt) -> JSContext object

      :param JSContext ctxt: an exists :py:class:`JSContext` instance
      :rtype: a cloned :py:class:`JSContext` instance

   .. automethod:: eval(source, name='', line=-1, col=-1, precompiled=None) -> object:

      Execute the Javascript code in source and return the result

      :param source: the Javascript code
      :type source: str or unicode
      :param str name: the name of the Javascript code
      :param integer line: the start line number of the Javascript code
      :param integer col: the start column number of the Javascript code
      :param buffer precompiled: the precompiled buffer of Javascript code
      :rtype: the result

   .. automethod:: __enter__() -> JSContext object

   .. automethod:: __exit__(exc_type, exc_value, traceback) -> None

   .. py:attribute:: current

       The context that is on the top of the stack.

   .. py:attribute:: entered

       The last entered context.

   .. py:attribute:: calling

       The context of the calling JavaScript code.

   .. py:attribute:: inContext

       Returns true if V8 has a current context.

JSExtension - reusable script module
------------------------------------
.. autoclass:: JSExtension
   :members:
   :inherited-members:

   .. automethod:: __init__(name, source, callback=None, dependencies=[], register=True) -> JSExtension object
   
      :param str name: the extension name
      :param str source: the Javascript code
      :param func callback: the native Python code
      :param list dependencies: the dependencies extensions
      :param bool register: register the extension
      :rtype: A reusable :py:class:`JSExtension` module
   
.. toctree::
   :maxdepth: 2

.. rubric:: Footnotes

.. [#f1] `ECMAScript <http://en.wikipedia.org/wiki/ECMAScript>`_ is the scripting language standardized by Ecma International in the ECMA-262 specification and ISO/IEC 16262. The language is widely used for client-side scripting on the web, in the form of several well-known dialects such as JavaScript, JScript, and ActionScript.

.. [#f2] There are three types of ECMAScript executable code:

         * *Global code* is source text that is treated as an ECMAScript *Program*.
         * *Eval code* is the source text supplied to the built-in **eval** function.
         * *Function code* is source text that is parsed as part of a *FunctionBody*.

.. [#f3] `Java Native Interface (JNI) <http://en.wikipedia.org/wiki/Java_Native_Interface>`_ is a programming framework that enables Java code running in a Java Virtual Machine (JVM) to call and to be called[1] by native applications (programs specific to a hardware and operating system platform) and libraries written in other languages such as C, C++ and assembly.