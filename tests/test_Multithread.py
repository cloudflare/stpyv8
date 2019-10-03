import sys
import unittest
import logging

import SoirV8


class TestMultithread(unittest.TestCase):
    def testLocker(self):
        with SoirV8.JSIsolate() as isolate:
            self.assertFalse(SoirV8.JSLocker.active)
            self.assertFalse(SoirV8.JSLocker.locked)

            with SoirV8.JSLocker() as outter_locker:
                self.assertTrue(SoirV8.JSLocker.active)
                self.assertTrue(SoirV8.JSLocker.locked)

                self.assertTrue(outter_locker)

                with SoirV8.JSLocker() as inner_locker:
                    self.assertTrue(SoirV8.JSLocker.locked)

                    self.assertTrue(outter_locker)
                    self.assertTrue(inner_locker)

                    with SoirV8.JSUnlocker() as unlocker:
                        self.assertFalse(SoirV8.JSLocker.locked)

                        self.assertTrue(outter_locker)
                        self.assertTrue(inner_locker)

                    self.assertTrue(SoirV8.JSLocker.locked)

            self.assertTrue(SoirV8.JSLocker.active)
            self.assertFalse(SoirV8.JSLocker.locked)

            locker = SoirV8.JSLocker()

        with SoirV8.JSContext():
            self.assertRaises(RuntimeError, locker.__enter__)
            self.assertRaises(RuntimeError, locker.__exit__, None, None, None)

        del locker

    def _testMultiPythonThread(self):
        import time, threading

        isolate = SoirV8.JSIsolate()
        isolate.enter()

        class Global:
            count = 0
            started = threading.Event()
            finished = threading.Semaphore(0)

            def sleep(self, ms):
                time.sleep(ms / 1000.0)

                self.count += 1

        g = Global()

        def run():
            with SoirV8.JSIsolate() as isolate:
                with SoirV8.JSContext(g) as ctxt:
                    ctxt.eval("""
                        started.wait();

                        for (i=0; i<10; i++)
                        {
                            sleep(100);
                        }

                        finished.release();
                    """)

        threading.Thread(target = run).start()

        now = time.time()

        self.assertEqual(0, g.count)
        
        g.started.set()
        g.finished.acquire()

        self.assertEqual(10, g.count)

        self.assertTrue((time.time() - now) >= 1)

    def _testMultiJavascriptThread(self):
        import time, threading

        isolate = SoirV8.JSIsolate()
        isolate.enter()

        class Global:
            result = []

            def add(self, value):
                with SoirV8.JSUnlocker():
                    time.sleep(0.1)

                    self.result.append(value)

        g = Global()

        def run():
            with SoirV8.JSContext(g) as ctxt:
                ctxt.eval("""
                    for (i=0; i<10; i++)
                        add(i);
                """)

        threads = [threading.Thread(target = run), threading.Thread(target = run)]

        with SoirV8.JSLocker():
            for t in threads: 
                t.start()

        for t in threads:
            t.join()

        self.assertEqual(20, len(g.result))


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
