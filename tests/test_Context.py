import sys
import unittest
import logging

import SoirV8


class TestContext(unittest.TestCase):
    def testEval(self):
        with SoirV8.JSContext() as context:
            self.assertEqual(2, context.eval("1+1"))
            self.assertEqual('Hello world', context.eval("'Hello ' + 'world'"))

    def testMultiNamespace(self):
        self.assertTrue(not bool(SoirV8.JSContext.inContext))
        self.assertTrue(not bool(SoirV8.JSContext.entered))

        class Global(object):
            name = "global"

        g = Global()

        with SoirV8.JSContext(g) as ctxt:
            self.assertTrue(ctxt)
            self.assertTrue(bool(SoirV8.JSContext.inContext))
            self.assertEqual(g.name, str(SoirV8.JSContext.entered.locals.name))

            class Local(object):
                name = "local"

            l = Local()

            with SoirV8.JSContext(l):
                self.assertTrue(bool(SoirV8.JSContext.inContext))
                self.assertEqual(l.name, str(SoirV8.JSContext.entered.locals.name))

            self.assertTrue(bool(SoirV8.JSContext.inContext))
            self.assertEqual(g.name, str(SoirV8.JSContext.current.locals.name))

        self.assertTrue(not bool(SoirV8.JSContext.entered))
        self.assertTrue(not bool(SoirV8.JSContext.inContext))

    def testMultiContext(self):
        with SoirV8.JSContext() as ctxt0:
            ctxt0.securityToken = "password"

            global0 = ctxt0.locals
            global0.custom = 1234

            self.assertEqual(1234, int(global0.custom))

            with SoirV8.JSContext() as ctxt1:
                ctxt1.securityToken = ctxt0.securityToken

                global1 = ctxt1.locals
                global1.custom = 1234

                with ctxt0:
                    self.assertEqual(1234, int(global0.custom))

                self.assertEqual(1234, int(global1.custom))

    def testSecurityChecks(self):
        with SoirV8.JSContext() as env1:
            env1.securityToken = "foo"

            # Create a function in env1.
            env1.eval("spy = function(){return spy;}")

            spy = env1.locals.spy

            self.assertTrue(isinstance(spy, SoirV8.JSFunction))

            # Create another function accessing global objects.
            env1.eval("spy2 = function(){return 123;}")

            spy2 = env1.locals.spy2

            self.assertTrue(isinstance(spy2, SoirV8.JSFunction))

            # Switch to env2 in the same domain and invoke spy on env2.
            env2 = SoirV8.JSContext()
            env2.securityToken = "foo"

            with env2:
                result = spy.apply(env2.locals)
                self.assertTrue(isinstance(result, SoirV8.JSFunction))

            env2.securityToken = "bar"

            # FIXME
            # Call cross_domain_call, it should throw an exception
            # with env2:
            #    self.assertRaises(SoirV8.JSError, spy2.apply, env2.locals)


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
