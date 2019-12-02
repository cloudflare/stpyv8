.. py:module:: PyV8

.. testsetup:: *

   from PyV8 import *
   
.. _engine:

Javascript Engine
==========================

Besides to execute a Javascript code with :py:meth:`JSContext.eval`, you could create a new :py:class:`JSEngine` instance and compile the Javascript code with :py:meth:`JSEngine.compile` before execute it. A :py:class:`JSScript` object will be returned, and you could run it later with :py:meth:`JSScript.run` method many times, or visit its AST [#f1]_ with :py:meth:`JSScript.visit`.

:py:class:`JSEngine` also contains some static properties and methods for the global v8 engine, for example:

======================================= =====================================================
Property or Method                      Description
======================================= =====================================================
:py:attr:`JSEngine.version`             Get the compiled v8 version
:py:attr:`JSEngine.dead`                Check if V8 is dead and therefore unusable
:py:meth:`JSEngine.collect`             Force a full garbage collection
:py:meth:`JSEngine.dispose`             Releases any resources used by v8
:py:attr:`JSEngine.currentThreadId`     Get the current v8 thread id
:py:meth:`JSEngine.terminateAllThreads` Forcefully terminate the current JavaScript thread
:py:meth:`JSEngine.terminateThread`     Forcefully terminate execution of a JavaScript thread
======================================= =====================================================

Compile Script and Control Engine
---------------------------------

When you use :py:meth:`JSEngine.compile` compile a Javascript code, the v8 engine will parse the sytanx and store the AST in a :py:class:`JSScript` object. You could execute it with :py:meth:`JSScript.run` or access the source code with :py:attr:`JSScript.source` later.

.. testcode::

    with JSContext() as ctxt:
        with JSEngine() as engine:
            s = engine.compile("1+2")

            print s.source # "1+2"
            print s.run()  # 3

.. testoutput::
   :hide:

   1+2
   3

You could only parse the sytanx with :py:meth:`JSEngine.precompile` before use it, which return a :py:class:`buffer` object contains some internal data. The buffer can't be executed directly, but could be used as the precompied parameter when call the :py:meth:`JSEngine.compile` later and improve the performance.

.. testcode::

        with JSContext() as ctxt:
            with JSEngine() as engine:
                buf = engine.precompile("1+2")

                # do something

                s = engine.compile("1+2", precompiled=buf) # use the parsed data to improve performancee

                print s.source # "1+2"
                print s.run()  # 3

.. testoutput::
   :hide:

   1+2
   3

If you need reuse the script in different contexts, you could refer to the :ref:`jsext`.

JSEngine - the backend Javascript engine
----------------------------------------
.. autoclass:: JSEngine
   :members:
   :inherited-members:
   :exclude-members: compile, precompile

   .. automethod:: compile(source, name='', line=-1, col=-1, precompiled=None) -> JSScript object

      Compile the Javascript code to a :py:class:`JSScript` object, which could be execute many times or visit it's AST.

      :param source: the Javascript code
      :type source: str or unicode
      :param str name: the name of the Javascript code
      :param integer line: the start line number of the Javascript code
      :param integer col: the start column number of the Javascript code
      :param buffer precompiled: the precompiled buffer of Javascript code
      :rtype: a compiled :py:class:`JSScript` object

   .. automethod:: precompile(source) -> buffer object

      Precompile the Javascript code to an internal buffer, which could be used to improve the performance when compile the same script later.

      :param source: the Javascript code
      :type source: str or unicode
      :rtype: a buffer object contains the precompiled internal data

   .. automethod:: __enter__() -> JSEngine object

   .. automethod:: __exit__(exc_type, exc_value, traceback) -> None

   .. py:attribute:: version

      Get the V8 engine version

   .. py:attribute:: dead

      Check if V8 is dead and therefore unusable.

   .. py:attribute:: currentThreadId

      The V8 thread id of the calling thread.

JSScript - the compiled script
------------------------------
.. autoclass:: JSScript
   :members:
   :inherited-members:
   :exclude-members: run, visit

   .. automethod:: run() -> object

   .. automethod:: visit(handler) -> None

      Please refer to the :ref:`ast` page for more detail.

.. toctree::
   :maxdepth: 2

.. rubric:: Footnotes

.. [#f1] `Abstract Syntax Tree (AST) <http://en.wikipedia.org/wiki/Abstract_syntax_tree>`_ is a tree representation of the abstract syntactic structure of source code written in a programming language. Each node of the tree denotes a construct occurring in the source code. The syntax is 'abstract' in the sense that it does not represent every detail that appears in the real syntax. For instance, grouping parentheses are implicit in the tree structure, and a syntactic construct such as an if-condition-then expression may be denoted by a single node with two branches.
