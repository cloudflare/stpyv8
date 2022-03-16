.. _build:

Build and Install
=================

Requirements
------------

Boost
^^^^^

STPyV8 makes use of `Boost.Python <http://www.boost.org/doc/libs/release/libs/python/doc/>`_ for interoperability.

Most Linux distributions provide easy to install Boost packages and this is the suggested way to install the library.
If packages are not available for your distribution download or install `the latest version 
<http://www.boost.org/users/download/>`_ of Boost and follow `the getting started guide 
<http://www.boost.org/doc/libs/release/more/getting_started/>`_ to build the library.

Python wheels
^^^^^^^^^^^^^

The easiest way to install STPyV8 is a Python wheel provided at `STPyV8 Releases <https://github.com/area1/stpyv8/releases>`_.
Such wheels are automatically generated using Github Actions and multiple platforms and Python versions are already
supported, with others planned for the future.

Each zip file contains the ICU data file icudtl.dat and the wheel itself.

First of all you should copy icudtl.data to the STPyV8 ICU data folder (Linux: /usr/share/stpyv8, MacOS:
/Library/Application Support/STPyV8/) and then install/upgrade STPyV8 using pip.

For instance, the following steps show how to install on MacOS

.. code-block:: sh

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

If no wheels are provided for your platform and Python version you are required to build STPyV8.

Build Steps
-----------

.. code-block:: sh

    $ python setup.py build
    $ sudo python setup.py install

Optionally you can run STPyV8 tests (pytest is required)

.. code-block:: sh

    $ pytest tests

If you want to build a distribution package for Linux/Mac run setup.py with the bdist command

.. code-block:: sh

    $ python setup.py bdist
