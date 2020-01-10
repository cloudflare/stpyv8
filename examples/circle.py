#!/usr/bin/env python

# circle.py - call a method in a javascript class from python

import STPyV8

with STPyV8.JSContext() as ctxt:
  ctxt.eval("""
    class Circle {
      constructor(radius) {
        this.radius = radius;
      }
      get area() {
        return this.calcArea()
      }
      calcArea() {
        return 3.14 * this.radius * this.radius;
      }
  }
  """)
  circle = ctxt.eval("new Circle(10)")
  print("Area of the circle: " + str(circle.area))
