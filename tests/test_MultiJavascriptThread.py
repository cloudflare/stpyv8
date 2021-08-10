import sys
import unittest
import logging

import STPyV8


class TestMultiJavascriptThread(unittest.TestCase):
    def testMultiJavascriptThread(self):
        import time, threading

        class Global(STPyV8.JSContext):
            result = []

            def add(self, value):
                with STPyV8.JSUnlocker():
                    # time.sleep(0.1)
                    self.result.append(value)

        g = Global()

        def run():
            with STPyV8.JSIsolate() as isolate:
                with STPyV8.JSContext(g) as ctxt:
                    ctxt.eval("""
                        for (i=0; i<10; i++)
                            add(i);
                    """)

        threads = [threading.Thread(target = run), threading.Thread(target = run)]

        with STPyV8.JSLocker():
            for t in threads:
                t.start()

        for t in threads:
            t.join()

        self.assertEqual(20, len(g.result))


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
