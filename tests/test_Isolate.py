import sys
import unittest
import logging

import SoirV8 

class TestIsolate(unittest.TestCase):
    def testBase(self):
        with SoirV8.JSIsolate() as isolate:
            self.assertIsNotNone(isolate.current)
            self.assertFalse(isolate.locked)


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()