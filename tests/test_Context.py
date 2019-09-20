import sys
import unittest
import logging

from SoirV8 import JSContext
from SoirV8 import JSIsolate
from SoirV8 import JSPlatform

class TestContext(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.platform = JSPlatform()
        self.platform.init()

        self.isolate = JSIsolate()
        self.isolate.enter()  #TODO remove?

    def testBase(self):
        context = JSContext()

        self.assertIsNone(context.entered)
        self.assertIsNone(context.current)
        self.assertIsNone(context.calling)
        self.assertFalse(context.inContext)

        with context:
            self.assertTrue(context.inContext)

    def testEval(self):
        with JSContext() as context:
            self.assertEqual(2, context.eval("1+1"))
            self.assertEqual('Hello world', context.eval("'Hello ' + 'world'"))

    def testCopy(self):
        context  = JSContext()
        context2 = JSContext(context)

        with context:
            self.assertEqual(2, context2.eval("1+1"))

    def testGlobal(self):
        class Global(object):
            name = "global"

        g = Global()

        with JSContext(g) as ctxt:
            self.assertTrue(bool(JSContext.inContext))


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
