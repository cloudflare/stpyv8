#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import with_statement
from __future__ import print_function

import os
import sys
import re
import collections.abc

import _STPyV8

__version__ = _STPyV8.JSEngine.version

__all__ = [
    "ReadOnly",
    "DontEnum",
    "DontDelete",
    "Internal",
    "JSError",
    "JSObject",
    "JSNull",
    "JSUndefined",
    "JSArray",
    "JSFunction",
    "JSClass",
    "JSEngine",
    "JSContext",
    "JSIsolate",
    "JSStackTrace",
    "JSStackFrame",
    "JSScript",
    "JSLocker",
    "JSUnlocker",
    "JSPlatform",
]


# ICU
ICU_DATA_FOLDERS_UNIX = (
    "/usr/share/stpyv8",
    os.path.expanduser("~/.local/share/stpyv8"),
)
ICU_DATA_FOLDERS_OSX = (
    "/Library/Application Support/STPyV8",
    os.path.expanduser("~/Library/Application Support/STPyV8"),
)
ICU_DATA_FOLDERS_WINDOWS = (
    os.path.join(os.environ["PROGRAMDATA"], "STPyV8")
    if "PROGRAMDATA" in os.environ
    else None,
    os.path.join(os.environ["APPDATA"], "STPyV8") if "APPDATA" in os.environ else None,
)

icu_data_folders = None
if os.name in ("posix",):
    icu_data_folders = (
        ICU_DATA_FOLDERS_OSX if sys.platform in ("darwin",) else ICU_DATA_FOLDERS_UNIX
    )
else:
    icu_data_folders = ICU_DATA_FOLDERS_WINDOWS


class JSAttribute:
    def __init__(self, name):
        self.name = name

    def __call__(self, func):
        setattr(func, f"__{self.name}__", True)

        return func


ReadOnly = JSAttribute(name="readonly")
DontEnum = JSAttribute(name="dontenum")
DontDelete = JSAttribute(name="dontdel")
Internal = JSAttribute(name="internal")


class JSError(Exception):
    def __init__(self, impl):
        Exception.__init__(self)
        self._impl = impl

    def __str__(self):
        return str(self._impl)

    def __getattribute__(self, attr):
        impl = super().__getattribute__("_impl")

        try:
            return getattr(impl, attr)
        except AttributeError:
            return super().__getattribute__(attr)

    RE_FRAME = re.compile(
        r"\s+at\s(?:new\s)?(?P<func>.+)\s\((?P<file>[^:]+):?(?P<row>\d+)?:?(?P<col>\d+)?\)"
    )
    RE_FUNC = re.compile(r"\s+at\s(?:new\s)?(?P<func>.+)\s\((?P<file>[^\)]+)\)")
    RE_FILE = re.compile(r"\s+at\s(?P<file>[^:]+):?(?P<row>\d+)?:?(?P<col>\d+)?")

    @staticmethod
    def parse_stack(value):
        stack = []

        def int_or_nul(value):
            return int(value) if value else None

        for line in value.split("\n")[1:]:
            m = JSError.RE_FRAME.match(line)

            if m:
                stack.append(
                    (
                        m.group("func"),
                        m.group("file"),
                        int_or_nul(m.group("row")),
                        int_or_nul(m.group("col")),
                    )
                )
                continue

            m = JSError.RE_FUNC.match(line)

            if m:
                stack.append((m.group("func"), m.group("file"), None, None))
                continue

            m = JSError.RE_FILE.match(line)

            if m:
                stack.append(
                    (
                        None,
                        m.group("file"),
                        int_or_nul(m.group("row")),
                        int_or_nul(m.group("col")),
                    )
                )
                continue

            assert line

        return stack

    @property
    def frames(self):
        return self.parse_stack(self.stackTrace)


_STPyV8._JSError._jsclass = JSError  # pylint:disable=protected-access

JSObject = _STPyV8.JSObject
JSNull = _STPyV8.JSNull
JSUndefined = _STPyV8.JSUndefined
JSArray = _STPyV8.JSArray
JSFunction = _STPyV8.JSFunction
JSPlatform = _STPyV8.JSPlatform


class JSLocker(_STPyV8.JSLocker):
    def __enter__(self):
        self.enter()

        if JSContext.entered:
            self.leave()
            raise RuntimeError("Lock should be acquired before entering the context")

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if JSContext.entered:
            self.leave()
            raise RuntimeError("Lock should be released after leaving the context")

        self.leave()

    def __bool__(self):
        return self.entered()


class JSUnlocker(_STPyV8.JSUnlocker):
    def __enter__(self):
        self.enter()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()

    def __bool__(self):
        return self.entered()


class JSClass:
    __properties__ = {}
    __watchpoints__ = {}

    def __getattr__(self, name):
        if name == "constructor":
            return JSClassConstructor(self.__class__)

        if name == "prototype":
            return JSClassPrototype(self.__class__)

        prop = self.__dict__.setdefault("__properties__", {}).get(name, None)

        if prop and isinstance(prop[0], collections.abc.Callable):
            return prop[0]()

        raise AttributeError(name)

    def __setattr__(self, name, value):
        prop = self.__dict__.setdefault("__properties__", {}).get(name, None)

        if prop and isinstance(prop[1], collections.abc.Callable):
            return prop[1](value)

        return object.__setattr__(self, name, value)

    def toString(self):
        """
        Return the string representation of the object
        """
        return f"[object {self.__class__.__name__}]"

    def toLocaleString(self):
        """
        Return the string representation of the object as a string value
        appropriate to the environment current locale
        """
        return self.toString()

    def valueOf(self):
        """
        Return the primitive value of the object
        """
        return self

    def hasOwnProperty(self, name):
        """
        Return a boolean value indicating whether the object has a property
        with the specified name
        """
        return hasattr(self, name)

    def isPrototypeOf(self, obj):
        """
        Return a boolean value indicating whether the object exists in the
        prototype chain of another object
        """
        raise NotImplementedError()

    def __defineGetter__(self, name, getter):
        """
        Bind the object property to a function to be called when that property
        is looked up
        """
        self.__properties__[name] = (getter, self.__lookupSetter__(name))

    def __lookupGetter__(self, name):
        """
        Return the function bound as a getter to the specified property
        """
        return self.__properties__.get(name, (None, None))[0]

    def __defineSetter__(self, name, setter):
        """
        Bind the object property to a function to be called when an attempt
        is made to set that property
        """
        self.__properties__[name] = (self.__lookupGetter__(name), setter)

    def __lookupSetter__(self, name):
        """
        Return the function bound as setter to the specified property
        """
        return self.__properties__.get(name, (None, None))[1]

    def watch(self, prop, handler):
        """
        Watch for a property to be assigned a value and runs a function when
        such assignment occurs
        """
        self.__watchpoints__[prop] = handler

    def unwatch(self, prop):
        """
        Remove a watchpoint set with the watch method
        """
        del self.__watchpoints__[prop]


class JSClassConstructor(JSClass):  # pylint:disable=abstract-method
    def __init__(self, cls):
        self.cls = cls

    @property
    def name(self):
        return self.cls.__name__

    def toString(self):
        return f"function {self.name}() {{\n  [native code]\n}}"

    def __call__(self, *args, **kwds):
        return self.cls(*args, **kwds)


class JSClassPrototype(JSClass):  # pylint:disable=abstract-method
    def __init__(self, cls):
        self.cls = cls

    @property
    def constructor(self):
        return JSClassConstructor(self.cls)

    @property
    def name(self):
        return self.cls.__name__


class JSEngine(_STPyV8.JSEngine):
    def __init__(self):
        _STPyV8.JSEngine.__init__(self)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        del self


JSScript = _STPyV8.JSScript
JSStackTrace = _STPyV8.JSStackTrace
JSStackTrace.Options = _STPyV8.JSStackTraceOptions
JSStackTrace.GetCurrentStackTrace = staticmethod(
    lambda frame_limit,  # pylint:disable=unnecessary-lambda
    options: _STPyV8.JSIsolate.current.GetCurrentStackTrace(frame_limit, options)
)
JSStackFrame = _STPyV8.JSStackFrame


class JSIsolate(_STPyV8.JSIsolate):
    def __enter__(self):
        self.enter()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()
        del self


class JSContext(_STPyV8.JSContext):
    def __init__(self, obj=None, ctxt=None):
        self.lock = JSLocker()
        self.lock.enter()

        if ctxt:
            _STPyV8.JSContext.__init__(self, ctxt)
        else:
            _STPyV8.JSContext.__init__(self, obj)

    def __enter__(self):
        self.enter()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()

        if hasattr(JSLocker, "lock"):
            self.lock.leave()
            self.lock = None

        del self


def icu_sync():
    if sys.version_info < (3, 10):
        from importlib_resources import files
    else:
        from importlib.resources import files

    for folder in icu_data_folders:
        if not folder or not os.path.exists(folder):
            continue

        version_file = os.path.join(folder, "stpyv8-version.txt")
        if not os.path.exists(version_file):
            continue

        with open(version_file, encoding="utf-8", mode="r") as fd:
            version = fd.read()

        if version.strip() in (__version__,):
            return

    try:
        stpyv8_icu_files = files("stpyv8-icu")
    except ModuleNotFoundError:
        return

    for f in stpyv8_icu_files.iterdir():
        if f.name not in ("icudtl.dat",):
            continue

        data = f.read_bytes()

        for folder in icu_data_folders:
            try:
                os.makedirs(folder, exist_ok=True)
                with open(os.path.join(folder, "icudtl.dat"), mode="wb") as fd:
                    fd.write(data)

                version_file = os.path.join(folder, "stpyv8-version.txt")
                with open(version_file, encoding="utf-8", mode="w") as fd:
                    fd.write(__version__)
            except PermissionError:
                pass


icu_sync()

v8_default_platform = JSPlatform()
v8_default_platform.init()

v8_default_isolate = JSIsolate()
v8_default_isolate.enter()
