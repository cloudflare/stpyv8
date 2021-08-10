#!/usr/bin/env python
# -*- coding: utf-8 -*-

import unittest

import STPyV8


class TestIsolate(unittest.TestCase):
    def testBase(self):
        with STPyV8.JSIsolate() as isolate:
            self.assertIsNotNone(isolate.current)
            self.assertFalse(isolate.locked)

    def testEnterLeave(self):
        with STPyV8.JSIsolate() as isolate:
            self.assertIsNotNone(isolate.current)
