#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import with_statement
from __future__ import print_function

import sys, os, re
import logging
import collections
import functools
import json

import six
from six.moves import input
from six.moves import _thread as thread

import _SoirV8

__version__ = '1.0'

__all__ = ["ReadOnly",
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
           "JSObjectSpace",
           "JSAllocationAction",
           "JSStackTrace",
           "JSStackFrame",
           "profiler",
           "JSExtension",
           "JSLocker",
           "JSUnlocker",
           "JSPlatform"]


class JSAttribute(object):
    def __init__(self, name):
        self.name = name

    def __call__(self, func):
        setattr(func, "__%s__" % self.name, True)

        return func

ReadOnly   = JSAttribute(name = 'readonly')
DontEnum   = JSAttribute(name = 'dontenum')
DontDelete = JSAttribute(name = 'dontdel')
Internal   = JSAttribute(name = 'internal')


class JSError(Exception):
    def __init__(self, impl):
        Exception.__init__(self)

        self._impl = impl

    def __str__(self):
        return str(self._impl)

    def __unicode__(self, *args, **kwargs):
        return six.u(self._impl)

    def __getattribute__(self, attr):
        impl = super(JSError, self).__getattribute__("_impl")

        try:
            return getattr(impl, attr)
        except AttributeError:
            return super(JSError, self).__getattribute__(attr)

    RE_FRAME = re.compile(r"\s+at\s(?:new\s)?(?P<func>.+)\s\((?P<file>[^:]+):?(?P<row>\d+)?:?(?P<col>\d+)?\)")
    RE_FUNC  = re.compile(r"\s+at\s(?:new\s)?(?P<func>.+)\s\((?P<file>[^\)]+)\)")
    RE_FILE  = re.compile(r"\s+at\s(?P<file>[^:]+):?(?P<row>\d+)?:?(?P<col>\d+)?")

    @staticmethod
    def parse_stack(value):
        stack = []

        def int_or_nul(value):
            return int(value) if value else None

        for line in value.split('\n')[1:]:
            m = JSError.RE_FRAME.match(line)

            if m:
                stack.append((m.group('func'), m.group('file'), int_or_nul(m.group('row')), int_or_nul(m.group('col'))))
                continue

            m = JSError.RE_FUNC.match(line)

            if m:
                stack.append((m.group('func'), m.group('file'), None, None))
                continue

            m = JSError.RE_FILE.match(line)

            if m:
                stack.append((None, m.group('file'), int_or_nul(m.group('row')), int_or_nul(m.group('col'))))
                continue

            assert line

        return stack

    @property
    def frames(self):
        return self.parse_stack(self.stackTrace)

_SoirV8._JSError._jsclass = JSError

JSObject    = _SoirV8.JSObject
JSNull      = _SoirV8.JSNull
JSUndefined = _SoirV8.JSUndefined
JSArray     = _SoirV8.JSArray
JSFunction  = _SoirV8.JSFunction
JSPlatform  = _SoirV8.JSPlatform


JS_ESCAPABLE = re.compile(r'([^\x00-\x7f])')
HAS_UTF8     = re.compile(r'[\x80-\xff]')


def _js_escape_unicode_re_callack(match):
    n = ord(match.group(0))
    if n < 0x10000:
        return '\\u%04x' % (n,)
    else:
        # surrogate pair
        n -= 0x10000
        s1 = 0xd800 | ((n >> 10) & 0x3ff)
        s2 = 0xdc00 | (n & 0x3ff)
        return '\\u%04x\\u%04x' % (s1, s2)


def js_escape_unicode(text):
    """Return an ASCII-only representation of a JavaScript string"""
    if isinstance(text, str):
        if HAS_UTF8.search(text) is None:
            return text

        text = text.decode('UTF-8')

    return str(JS_ESCAPABLE.sub(_js_escape_unicode_re_callack, text))

'''
class JSExtension(_SoirV8.JSExtension):
    def __init__(self, name, source, callback=None, dependencies=[], register=True):
        _SoirV8.JSExtension.__init__(self, js_escape_unicode(name), js_escape_unicode(source), callback, dependencies, register)


class JSLocker(_SoirV8.JSLocker):
    def __enter__(self):
        self.enter()

        if JSContext.entered:
            self.leave()
            raise RuntimeError("Lock should be acquired before enter the context")

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if JSContext.entered:
            self.leave()
            raise RuntimeError("Lock should be released after leave the context")

        self.leave()

    if is_py3k:
        def __bool__(self):
            return self.entered()
    else:
        def __nonzero__(self):
            return self.entered()


class JSUnlocker(_SoirV8.JSUnlocker):
    def __enter__(self):
        self.enter()

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()

    if is_py3k:
        def __bool__(self):
            return self.entered()
    else:
        def __nonzero__(self):
            return self.entered()
'''

class JSClass(object):
    __properties__ = {}
    __watchpoints__ = {}

    def __getattr__(self, name):
        if name == 'constructor':
            return JSClassConstructor(self.__class__)

        if name == 'prototype':
            return JSClassPrototype(self.__class__)

        prop = self.__dict__.setdefault('__properties__', {}).get(name, None)

        if prop and isinstance(prop[0], collections.Callable):
            return prop[0]()

        raise AttributeError(name)

    def __setattr__(self, name, value):
        prop = self.__dict__.setdefault('__properties__', {}).get(name, None)

        if prop and isinstance(prop[1], collections.Callable):
            return prop[1](value)

        return object.__setattr__(self, name, value)

    def toString(self):
        "Returns a string representation of an object."
        return "[object %s]" % self.__class__.__name__

    def toLocaleString(self):
        "Returns a value as a string value appropriate to the host environment's current locale."
        return self.toString()

    def valueOf(self):
        "Returns the primitive value of the specified object."
        return self

    def hasOwnProperty(self, name):
        "Returns a Boolean value indicating whether an object has a property with the specified name."
        return hasattr(self, name)

    def isPrototypeOf(self, obj):
        "Returns a Boolean value indicating whether an object exists in the prototype chain of another object."
        raise NotImplementedError()

    def __defineGetter__(self, name, getter):
        "Binds an object's property to a function to be called when that property is looked up."
        self.__properties__[name] = (getter, self.__lookupSetter__(name))

    def __lookupGetter__(self, name):
        "Return the function bound as a getter to the specified property."
        return self.__properties__.get(name, (None, None))[0]

    def __defineSetter__(self, name, setter):
        "Binds an object's property to a function to be called when an attempt is made to set that property."
        self.__properties__[name] = (self.__lookupGetter__(name), setter)

    def __lookupSetter__(self, name):
        "Return the function bound as a setter to the specified property."
        return self.__properties__.get(name, (None, None))[1]

    def watch(self, prop, handler):
        "Watches for a property to be assigned a value and runs a function when that occurs."
        self.__watchpoints__[prop] = handler

    def unwatch(self, prop):
        "Removes a watchpoint set with the watch method."
        del self.__watchpoints__[prop]


class JSClassConstructor(JSClass):
    def __init__(self, cls):
        self.cls = cls

    @property
    def name(self):
        return self.cls.__name__

    def toString(self):
        return "function %s() {\n  [native code]\n}" % self.name

    def __call__(self, *args, **kwds):
        return self.cls(*args, **kwds)


class JSClassPrototype(JSClass):
    def __init__(self, cls):
        self.cls = cls

    @property
    def constructor(self):
        return JSClassConstructor(self.cls)

    @property
    def name(self):
        return self.cls.__name__


# JSObjectSpace = _SoirV8.JSObjectSpace
# JSAllocationAction = _SoirV8.JSAllocationAction


class JSEngine(_SoirV8.JSEngine):
    def __init__(self):
        _SoirV8.JSEngine.__init__(self)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        del self

JSScript = _SoirV8.JSScript

JSStackTrace = _SoirV8.JSStackTrace
# JSStackTrace.Options = _SoirV8.JSStackTraceOptions
JSStackTrace.GetCurrentStackTrace = staticmethod(lambda frame_limit, options: _SoirV8.JSIsolate.current.GetCurrentStackTrace(frame_limit, options))
JSStackFrame = _SoirV8.JSStackFrame


class JSIsolate(_SoirV8.JSIsolate):
    def __enter__(self):
        self.enter()

        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()

        del self


class JSContext(_SoirV8.JSContext):
    def __init__(self, obj = None, extensions = None, ctxt = None):
        # if JSLocker.active:
        #    self.lock = JSLocker()
        #    self.lock.enter()
        if ctxt:
            _SoirV8.JSContext.__init__(self, ctxt)
        else:
            _SoirV8.JSContext.__init__(self, obj, extensions or [])

    def __enter__(self):
        self.enter()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.leave()

        # if hasattr(JSLocker, 'lock'):
        #    self.lock.leave()
        #    self.lock = None

        del self
