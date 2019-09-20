import sys
import unittest
import logging

import _SoirV8

class TestContext(unittest.TestCase):
   
    plat = None
    isolate = None

    def makeplat(self):
        if(self.plat is None):
            self.plat =  _SoirV8.JSPlatform()
            self.plat.init()
        if(self.isolate is None):
            self.isolate = _SoirV8.JSIsolate()
            self.isolate.enter()  #TODO remove?

    def testBase(self):
        self.makeplat()
        context = _SoirV8.JSContext()
        self.assertIsNone(context.entered)
        self.assertIsNone(context.current)
        self.assertIsNone(context.calling)
        self.assertFalse(context.inContext)
        context.enter()
        context.leave()

    def testEval(self):
        self.makeplat()
        context = _SoirV8.JSContext()
        context.enter()
        self.assertEqual(2, context.eval("1+1"))
        self.assertEqual('Hello world', context.eval("'Hello ' + 'world'"))
        context.leave()

    def testCopy(self):
        self.makeplat()
        context = _SoirV8.JSContext()
        context2 = _SoirV8.JSContext(context)
        context.enter()
        self.assertEqual(2, context2.eval("1+1"))
 
if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
