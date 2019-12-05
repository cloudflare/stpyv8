#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
import unittest
import logging

import SpyV8


class TestEngine(unittest.TestCase):
    def testClassProperties(self):
        with SpyV8.JSContext():
            self.assertTrue(str(SpyV8.JSEngine.version).startswith("7."))
            self.assertFalse(SpyV8.JSEngine.dead)

    def testCompile(self):
        with SpyV8.JSContext():
            with SpyV8.JSEngine() as engine:
                s = engine.compile("1+2")

                self.assertTrue(isinstance(s, SpyV8.JSScript))

                self.assertEqual("1+2", s.source)
                self.assertEqual(3, int(s.run()))

                self.assertRaises(SyntaxError, engine.compile, "1+")

    def testUnicodeSource(self):
        class Global(SpyV8.JSClass):
            var = u'测试'

            def __getattr__(self, name):
                if name:
                    return self.var

                return SpyV8.JSClass.__getattr__(self, name)

        g = Global()

        with SpyV8.JSContext(g) as ctxt:
            with SpyV8.JSEngine() as engine:
                src = u"""
                function 函数() { return 变量.length; }

                函数();

                var func = function () {};
                """

                s = engine.compile(src)

                self.assertTrue(isinstance(s, SpyV8.JSScript))

                self.assertEqual(src, s.source)
                self.assertEqual(2, s.run())

                func_name = u'函数'

                self.assertTrue(hasattr(ctxt.locals, func_name))

                func = getattr(ctxt.locals, func_name)

                self.assertTrue(isinstance(func, SpyV8.JSFunction))

                self.assertEqual(func_name, func.name)
                self.assertEqual("", func.resname)
                self.assertEqual(1, func.linenum)
                self.assertEqual(0, func.lineoff)
                self.assertEqual(0, func.coloff)

                var_name = u'变量'

                setattr(ctxt.locals, var_name, u'测试长字符串')

                self.assertEqual(6, func())

                self.assertEqual("func", ctxt.locals.func.inferredname)

    def testEval(self):
        with SpyV8.JSContext() as ctxt:
            self.assertEqual(3, int(ctxt.eval("1+2")))

    def testGlobal(self):
        class Global(SpyV8.JSClass):
            version = "1.0"

        with SpyV8.JSContext(Global()) as ctxt:
            _vars = ctxt.locals

            # getter
            self.assertEqual(Global.version, str(_vars.version))
            self.assertEqual(Global.version, str(ctxt.eval("version")))

            self.assertRaises(ReferenceError, ctxt.eval, "nonexists")

            # setter
            self.assertEqual(2.0, float(ctxt.eval("version = 2.0")))

            self.assertEqual(2.0, float(_vars.version))

    def testThis(self):
        class Global(SpyV8.JSClass):
            version = 1.0

        with SpyV8.JSContext(Global()) as ctxt:
            self.assertEqual("[object Global]", str(ctxt.eval("this")))
            self.assertEqual(1.0, float(ctxt.eval("this.version")))

    def testObjectBuiltInMethods(self):
        class Global(SpyV8.JSClass):
            version = 1.0

        with SpyV8.JSContext(Global()) as ctxt:
            self.assertEqual("[object Global]", str(ctxt.eval("this.toString()")))
            self.assertEqual("[object Global]", str(ctxt.eval("this.toLocaleString()")))
            self.assertEqual(Global.version, float(ctxt.eval("this.valueOf()").version))

            self.assertTrue(bool(ctxt.eval("this.hasOwnProperty(\"version\")")))

            self.assertFalse(ctxt.eval("this.hasOwnProperty(\"nonexistent\")"))

    def testPythonWrapper(self):
        class Global(SpyV8.JSClass):
            s = [1, 2, 3]
            d = {'a': {'b': 'c'}, 'd': ['e', 'f']}

        g = Global()

        with SpyV8.JSContext(g) as ctxt:
            ctxt.eval("""
                s[2] = s[1] + 2;
                s[0] = s[1];
                delete s[1];
            """)
            self.assertEqual([2, 4], g.s)
            self.assertEqual('c', ctxt.eval("d.a.b"))
            self.assertEqual(['e', 'f'], ctxt.eval("d.d"))
            ctxt.eval("""
                d.a.q = 4
                delete d.d
            """)
            self.assertEqual(4, g.d['a']['q'])
            self.assertEqual(None, ctxt.eval("d.d"))

    def _testMemoryAllocationCallback(self):
        alloc = {}

        def callback(space, action, size):
            alloc[(space, action)] = alloc.setdefault((space, action), 0) + size

        SpyV8.JSEngine.setMemoryAllocationCallback(callback)

        with SpyV8.JSContext() as ctxt:
            self.assertFalse((SpyV8.JSObjectSpace.Code, SpyV8.JSAllocationAction.alloc) in alloc)

            ctxt.eval("var o = new Array(1000);")

            self.assertTrue((SpyV8.JSObjectSpace.Code, SpyV8.JSAllocationAction.alloc) in alloc)

        SpyV8.JSEngine.setMemoryAllocationCallback(None)

    def _testOutOfMemory(self):
        with SpyV8.JSIsolate():
            SpyV8.JSEngine.setMemoryLimit(max_young_space_size=16 * 1024, max_old_space_size=4 * 1024 * 1024)

            with SpyV8.JSContext() as ctxt:
                SpyV8.JSEngine.ignoreOutOfMemoryException()

                ctxt.eval("var a = new Array(); while(true) a.push(a);")

                self.assertTrue(ctxt.hasOutOfMemoryException)

                SpyV8.JSEngine.setMemoryLimit()

                SpyV8.JSEngine.collect()

    def testStackLimit(self):
        with SpyV8.JSIsolate():
            SpyV8.JSEngine.setStackLimit(256 * 1024)

            with SpyV8.JSContext() as ctxt:
                oldStackSize = ctxt.eval("var maxStackSize = function(i){try{(function m(){++i&&m()}())}catch(e){return i}}(0); maxStackSize")

        with SpyV8.JSIsolate():
            SpyV8.JSEngine.setStackLimit(512 * 1024)

            with SpyV8.JSContext() as ctxt:
                newStackSize = ctxt.eval("var maxStackSize = function(i){try{(function m(){++i&&m()}())}catch(e){return i}}(0); maxStackSize")

        self.assertTrue(newStackSize > oldStackSize * 2)


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
