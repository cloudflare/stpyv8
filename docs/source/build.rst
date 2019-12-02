.. _build:

Build and Install
=================

Requirements
------------

Boost
^^^^^

SoirV8 makes use of `boost.python <http://www.boost.org/doc/libs/release/libs/python/doc/>`_ [#f2]_ for interoperability.

Please download or install `the latest version <http://www.boost.org/users/download/>`_ of Boost and follow `the getting
started guide <http://www.boost.org/doc/libs/release/more/getting_started/>`_ to build the library. Most Linux distributions
provide easy to install Boost packages and this is the suggested way to install the library.

Build Steps
-----------

SoirV8 build system attempts to automate all the required installation steps but at the moment it is not entirely possible
because Google depot_tools requires Python 2.7. For this reason the installation currently requires some extra steps which
hopefully will not be required in the next future. Moreover you are required to install both Python 2.7 and Python 3.6+ on
your system. 

Note: if installing SoirV8 on a Debian/Ubuntu Linux distribution the install script install-ubuntu.sh will take care of all
the installation steps. In such case you are highly suggested to execute the script and skip the following sections.

Build V8
^^^^^^^^

First of all switch to Python 2.7. The steps required to do that are not provided here because they are different based on
your platform (and distribution). 

Running the setup.py with the v8 command will build and install the latest stable version of Google V8

.. code-block:: sh

    $ python setup.py v8     # build and install Google V8


Build SoirV8
^^^^^^^^^^^^

Switch to Python 3.6+ and run the following commands to install SoirV8

.. code-block:: sh

    $ python setup.py soirv8        # build SoirV8
    $ sudo python setup.py install  # install SoirV8


Optionally you can run SoirV8 tests (pytest is required)

.. code-block:: sh

    $ pytest tests/ # run SoirV8 tests


If you want to build a distribution package for Linux/Mac run setup.py with the bdist command

.. code-block:: sh

    $ python setup.py bdist           # build the distribution package for Linux/Mac
