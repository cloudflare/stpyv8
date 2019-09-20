import sys
import unittest
import logging

from SoirV8 import JSIsolate
from SoirV8 import JSPlatform


class TestIsolate(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.platform = JSPlatform()
        self.platform.init()

    def testBase(self):
        with JSIsolate() as isolate:
            self.assertIsNotNone(isolate.current)
            self.assertFalse(isolate.locked)

    def testEnterLeave(self):
        isolate = JSIsolate()

        isolate.enter()
        self.assertIsNotNone(isolate.current)

        isolate.leave()
        self.assertIsNotNone(isolate.current)

if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
