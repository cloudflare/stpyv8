import sys
import unittest
import logging

import SoirV8 

class TestIsolate(unittest.TestCase):
    def testBase(self):
        with SoirV8.JSIsolate() as isolate:
            isolate.enter()
            self.assertIsNotNone(isolate.current)
            self.assertFalse(isolate.locked)


if __name__ == '__main__':
    if "-v" in sys.argv:
        level = logging.DEBUG
    else:
        level = logging.WARN

    logging.basicConfig(level=level, format = '%(asctime)s %(levelname)s %(message)s')
    #logging.info("Testing PyV8 module with V8 v%s", JSEngine.version)

    unittest.main()
