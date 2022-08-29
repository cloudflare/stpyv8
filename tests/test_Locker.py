#!/usr/bin/env python
# -*- coding: utf-8 -*-

import unittest

import STPyV8


class TestLocker(unittest.TestCase):
    def testLocker(self):
        with STPyV8.JSIsolate():
            self.assertFalse(STPyV8.JSLocker.locked)

            with STPyV8.JSLocker() as outter_locker:
                self.assertTrue(STPyV8.JSLocker.locked)

                self.assertTrue(outter_locker)

                with STPyV8.JSLocker() as inner_locker:
                    self.assertTrue(STPyV8.JSLocker.locked)

                    self.assertTrue(outter_locker)
                    self.assertTrue(inner_locker)

                    with STPyV8.JSUnlocker():
                        self.assertFalse(STPyV8.JSLocker.locked)

                        self.assertTrue(outter_locker)
                        self.assertTrue(inner_locker)

                    self.assertTrue(STPyV8.JSLocker.locked)

            self.assertFalse(STPyV8.JSLocker.locked)

            locker = STPyV8.JSLocker()

        with STPyV8.JSContext():
            self.assertRaises(RuntimeError, locker.__enter__)
            self.assertRaises(RuntimeError, locker.__exit__, None, None, None)

        del locker
