#!/usr/bin/env python
# -*- coding: utf-8 -*-

import time
import threading
import unittest
import pytest

import STPyV8


class TestThread(unittest.TestCase):
    @pytest.mark.order(1)
    def testMultiPythonThread(self):
        class Global:
            count = 0
            started = threading.Event()
            finished = threading.Semaphore(0)

            def sleep(self, ms):
                time.sleep(ms / 1000.0)

                self.count += 1

        g = Global()

        def run():
            with STPyV8.JSIsolate():
                with STPyV8.JSContext(g) as ctxt:
                    ctxt.eval(
                        """
                        started.wait();

                        for (i=0; i<10; i++)
                        {
                            sleep(100);
                        }

                        finished.release();
                    """
                    )

        t = threading.Thread(target=run)
        t.start()

        now = time.time()

        self.assertEqual(0, g.count)

        g.started.set()
        g.finished.acquire()

        self.assertEqual(10, g.count)

        self.assertTrue((time.time() - now) >= 1)
        t.join()

    @pytest.mark.order(2)
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
                    ctxt.eval(
                        """
                        for (i=0; i<10; i++)
                            add(i);
                    """
                    )

        threads = [threading.Thread(target=run), threading.Thread(target=run)]

        with STPyV8.JSLocker():
            for t in threads:
                t.start()

        for t in threads:
            t.join()

        self.assertEqual(20, len(g.result))
