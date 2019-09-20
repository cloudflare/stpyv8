import sys
import unittest
import logging

import _SoirV8

class TestIsolate(unittest.TestCase):
   
    plat = None

    def makeplat(self):
        if(self.plat is None):
            self.plat =  _SoirV8.JSPlatform()
            self.plat.init()

    def testBase(self):
        #with _SoirV8.JSIsolate() as isolate:   #TODO __exit__ not defined?
        self.makeplat()
        isolate = _SoirV8.JSIsolate()
        isolate.enter()     #TODO have to call enter first?
        self.assertIsNotNone(isolate.current)
        self.assertFalse(isolate.locked)

    def testEnterLeave(self):
        self.makeplat()
        isolate = _SoirV8.JSIsolate()
        curr = isolate.current
        isolate.enter()
        self.assertIsNotNone(isolate.current)
        isolate.leave()
        self.assertIsNotNone(isolate.current)

if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
