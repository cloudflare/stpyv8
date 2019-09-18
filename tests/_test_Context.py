import sys
import unittest
import logging

import SoirV8 


class TestContext(unittest.TestCase):
    def testEval1(self):
        # FIXME
        # SoirV8.JSContext should perform this operation through JSLocker
        with SoirV8.JSIsolate() as isolate:
            with SoirV8.JSContext() as ctxt:
                result = ctxt.eval("1 + 2")
                self.assertEquals(3, result)


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
