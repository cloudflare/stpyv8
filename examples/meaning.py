#!/usr/bin/env python

# meaning.py - call a python method from javascript code

import STPyV8

class MyClass(STPyV8.JSClass):
  def reallyComplexFunction(self, addme):
    return 10 * 3 + addme

my_class = MyClass()

with STPyV8.JSContext(my_class) as ctxt:
  meaning = ctxt.eval("this.reallyComplexFunction(2) + 10;")
  print("The meaning of life: " + str(meaning))

