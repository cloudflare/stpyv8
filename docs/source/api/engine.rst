.. py:module:: SpyV8

.. testsetup:: *

   from SpyV8 import *
   
.. _engine:

Javascript Engine
==========================

Besides executing Javascript code with the method :py:meth:`JSContext.eval`, you could create a new :py:class:`JSEngine` 
instance and compile the Javascript code using the method :py:meth:`JSEngine.compile`. A :py:class:`JSScript` object is
returned. This object allows code inspection and provides the method :py:meth:`JSScript.run` that allows to execute the
compiled code.

The class :py:class:`JSEngine` also provides the following static properties and methods

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

When you use the method :py:meth:`JSEngine.compile` to compile a Javascript code, the V8 engine will parse the syntax and
store the AST [#f1]_ in a :py:class:`JSScript` object. You could then execute it with the method :py:meth:`JSScript.run` or access 
the source code with the attribute :py:attr:`JSScript.source`.

.. testcode::

    with JSContext() as ctxt:
        with JSEngine() as engine:
            s = engine.compile("1+2")

            print(s.source) # "1+2"
            print(s.run())  # 3

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

   .. automethod:: compile(source, name = '', line = -1, col = -1) -> JSScript object

      Compile the Javascript code to a :py:class:`JSScript` object, which could be execute many times or visit it's AST.

      :param source: the Javascript code to be compiled
      :type source: str or unicode
      :param str name: the name of the Javascript code
      :param integer line: the start line number of the Javascript code
      :param integer col: the start column number of the Javascript code
      :rtype: a compiled :py:class:`JSScript` object

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

.. toctree::
   :maxdepth: 2

.. rubric:: Footnotes

.. [#f1] `Abstract Syntax Tree (AST) <http://en.wikipedia.org/wiki/Abstract_syntax_tree>`_ is a tree representation of the abstract syntactic structure of source code written in a programming language. Each node of the tree denotes a construct occurring in the source code. The syntax is 'abstract' in the sense that it does not represent every detail that appears in the real syntax. For instance, grouping parentheses are implicit in the tree structure, and a syntactic construct such as an if-condition-then expression may be denoted by a single node with two branches.
