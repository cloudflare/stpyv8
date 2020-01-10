#!/usr/bin/env python

# global.py - pass different global objects to javascript

import STPyV8 as V8

class Global(V8.JSClass):
  version = "1.0"

  def hello(self, name):
    return "Hello " + name

with V8.JSContext(Global()) as ctxt:
  print(ctxt.eval("version"))          # 1.0
  print(ctxt.eval("hello('World')"))   # Hello World
  print(ctxt.eval("hello.toString()")) # function () { [native code] }

# "simulate" a browser, defining the js 'window' object in python
# see https://github.com/buffer/thug for a robust implementation

class window(V8.JSClass):

  def alert(self, message):
    print("Popping up an alert with message " + message)

with V8.JSContext(window()) as browser_ctxt:
  browser_ctxt.eval("alert('Hello')")

