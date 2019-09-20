import sys
import unittest
import logging

from SoirV8 import JSContext
from SoirV8 import JSIsolate


class TestContext(unittest.TestCase):
    def _testEval(self):
        # FIXME
        # JSContext should perform this operation through JSLocker
        with JSIsolate() as isolate:
            with JSContext() as ctxt:
                result = ctxt.eval("1 + 2")
                self.assertEquals(3, result)

    def testMultiNamespace(self):
        # self.assertTrue(not bool(JSContext.inContext))
        # self.assertTrue(not bool(JSContext.entered))

        class Global(object):
            name = "global"

        g = Global()

        with JSIsolate() as isolate:
            with JSContext(g) as ctxt:
                self.assertTrue(bool(JSContext.inContext))
                self.assertEqual(None, JSContext.entered.locals)
                # self.assertEqual(g.name, str(JSContext.entered.locals.name))
                # self.assertEqual(g.name, str(JSContext.current.locals.name))

                print("HERE")
                class Local(object):
                    name = "local"

                l = Local()

                with JSContext(l):
                    self.assertTrue(bool(JSContext.inContext))
                    #self.assertEqual(l.name, str(JSContext.entered.locals.name))
                    #self.assertEqual(l.name, str(JSContext.current.locals.name))

                self.assertTrue(bool(JSContext.inContext))
                #self.assertEqual(g.name, str(JSContext.entered.locals.name))
                #self.assertEqual(g.name, str(JSContext.current.locals.name))

            self.assertTrue(not bool(JSContext.entered))
            self.assertTrue(not bool(JSContext.inContext))


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
