.. _build:

Build and Install
=================

Requirements
------------

Boost
^^^^^

SpyV8 makes use of `Boost.Python <http://www.boost.org/doc/libs/release/libs/python/doc/>`_ for interoperability.

Most Linux distributions provide easy to install Boost packages and this is the suggested way to install the library.
If packages are not available for your distribution download or install `the latest version 
<http://www.boost.org/users/download/>`_ of Boost and follow `the getting started guide 
<http://www.boost.org/doc/libs/release/more/getting_started/>`_ to build the library.

Build Steps
-----------

SpyV8 build system attempts to automate all the required installation steps but at the moment it is not entirely possible
because Google depot_tools requires Python 2.7. For this reason some extra steps are currently required. Hopefully these
additional steps will not be required in the next future and this documentation will be update accordingly. Meanwhile
both Python 2.7 and Python 3.6+ have to be installed on your system. 

Note: if installing SpyV8 on a Ubuntu/Debian Linux distribution the provided helper script *install-ubuntu.sh* will take
care of all the installation details. In such case just execute the script and skip the following sections.

Build V8
^^^^^^^^

First of all set the default Python version to Python 2.7. The steps required to do that are beyond the scope of this
documentation being different on almost every platform and distribution. A suggested approach is to use a Python version
management tool like `pyenv <https://github.com/pyenv/pyenv>`_ which could help easily switching between multiple versions
of Python.

Running setup.py with the v8 command will build and install the latest stable version of Google V8

.. code-block:: sh

    $ python setup.py v8  # build and install Google V8


Build SpyV8
^^^^^^^^^^^

Set the default Python version to Python 3.6+ and run the following commands to install SpyV8

.. code-block:: sh

    $ python setup.py spyv8        # build SpyV8
    $ sudo python setup.py install  # install SpyV8


Optionally you can run SpyV8 tests (pytest is required)

.. code-block:: sh

    $ pytest tests/ # run SpyV8 tests

If you want to build a distribution package for Linux/Mac run setup.py with the bdist command

.. code-block:: sh

    $ python setup.py bdist  # build the distribution package for Linux/Mac
