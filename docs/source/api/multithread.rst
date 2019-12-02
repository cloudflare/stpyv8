.. py:module:: PyV8

.. testsetup:: *

   from PyV8 import *

.. _multithread:

Multi-Thread and Lock
==========================

The Javascript Thread and Isolate
---------------------------------

V8 isolates have completely separate states. Objects from one isolate must not be used in other isolates.  When V8 is initialized a default isolate is implicitly created and entered.  The embedder can create additional isolates and use them in parallel in multiple threads.  An isolate can be entered by at most one thread at any given time.  The Locker/Unlocker API can be used to synchronize.


JSIsolate
---------

.. autoclass:: JSIsolate
   :members:
   :inherited-members:

   .. automethod:: __enter__() -> JSIsolate object

   .. automethod:: __exit__(exc_type, exc_value, traceback) -> None

.. toctree::
   :maxdepth: 2