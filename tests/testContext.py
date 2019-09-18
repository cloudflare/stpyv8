import sys
import unittest
import logging

import SoirV8 


class TestContext(unittest.TestCase):
    def _testEval1(self):
        with SoirV8.JSContext() as ctxt:
            result = ctxt.eval("1 + 2")
            self.assertEquals(3, result)


if __name__ == '__main__':
    if "-v" in sys.argv:
        level = logging.DEBUG
    else:
        level = logging.WARN

    logging.basicConfig(level=level, format = '%(asctime)s %(levelname)s %(message)s')
    #logging.info("Testing PyV8 module with V8 v%s", JSEngine.version)

    unittest.main()
