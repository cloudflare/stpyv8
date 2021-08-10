#!/usr/bin/env python
# -*- coding: utf-8 -*-

import threading
import unittest

import STPyV8


class TestMultiJavascriptThread(unittest.TestCase):
    def testMultiJavascriptThread(self):
        class Global(STPyV8.JSContext):
            result = []

            def add(self, value):
                with STPyV8.JSUnlocker():
                    self.result.append(value)

        g = Global()

        def run():
            with STPyV8.JSIsolate():
                with STPyV8.JSContext(g) as ctxt:
                    ctxt.eval("""
                        for (i=0; i<10; i++)
                            add(i);
                    """)

        threads = [threading.Thread(target = run),
                   threading.Thread(target = run)]

        with STPyV8.JSLocker():
            for t in threads:
                t.start()

        for t in threads:
            t.join()

        self.assertEqual(20, len(g.result))
