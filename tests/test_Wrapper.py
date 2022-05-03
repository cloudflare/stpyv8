#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
import datetime
import unittest

import STPyV8


def convert(obj):
    if isinstance(obj, STPyV8.JSArray):
        return [convert(v) for v in obj]

    if isinstance(obj, STPyV8.JSObject):
        return dict([[str(k), convert(obj.__getattr__(str(k)))] for k in obj.__dir__()])

    return obj


class TestWrapper(unittest.TestCase):
    def testObject(self):
        with STPyV8.JSContext() as ctxt:
            o = ctxt.eval("new Object()")

            self.assertTrue(hash(o) > 0)

            o1 = o.clone()

            self.assertEqual(hash(o1), hash(o))
            self.assertTrue(o != o1)

        self.assertRaises(UnboundLocalError, o.clone)

    def testAutoConverter(self):
        with STPyV8.JSContext() as ctxt:
            ctxt.eval("""
                var_i = 1;
                var_f = 1.0;
                var_s = "test";
                var_b = true;
                var_s_obj = new String("test");
                var_b_obj = new Boolean(true);
                var_f_obj = new Number(1.5);
            """)

            _vars = ctxt.locals

            var_i = _vars.var_i

            self.assertTrue(var_i)
            self.assertEqual(1, int(var_i))

            var_f = _vars.var_f

            self.assertTrue(var_f)
            self.assertEqual(1.0, float(_vars.var_f))

            var_s = _vars.var_s
            self.assertTrue(var_s)
            self.assertEqual("test", str(_vars.var_s))

            var_b = _vars.var_b
            self.assertTrue(var_b)
            self.assertTrue(bool(var_b))

            self.assertEqual("test", _vars.var_s_obj)
            self.assertTrue(_vars.var_b_obj)
            self.assertEqual(1.5, _vars.var_f_obj)

            attrs = dir(ctxt.locals)

            self.assertTrue(attrs)
            self.assertTrue("var_i" in attrs)
            self.assertTrue("var_f" in attrs)
            self.assertTrue("var_s" in attrs)
            self.assertTrue("var_b" in attrs)
            self.assertTrue("var_s_obj" in attrs)
            self.assertTrue("var_b_obj" in attrs)
            self.assertTrue("var_f_obj" in attrs)

    def testExactConverter(self):
        class MyInteger(int, STPyV8.JSClass):
            pass

        class MyString(str, STPyV8.JSClass):
            pass

        class MyDateTime(datetime.time, STPyV8.JSClass):
            pass

        class Global(STPyV8.JSClass):
            var_bool = True
            var_int = 1
            var_float = 1.0
            var_str = 'str'
            var_datetime = datetime.datetime.now()
            var_date = datetime.date.today()
            var_time = datetime.time()

            var_myint = MyInteger()
            var_mystr = MyString('mystr')
            var_mytime = MyDateTime()

        with STPyV8.JSContext(Global()) as ctxt:
            typename = ctxt.eval("(function (name) { return this[name].constructor.name; })")
            typeof = ctxt.eval("(function (name) { return typeof(this[name]); })")

            self.assertEqual('Boolean', typename('var_bool'))
            self.assertEqual('Number', typename('var_int'))
            self.assertEqual('Number', typename('var_float'))
            self.assertEqual('String', typename('var_str'))
            self.assertEqual('Date', typename('var_datetime'))
            self.assertEqual('Date', typename('var_date'))
            self.assertEqual('Date', typename('var_time'))

            self.assertEqual('MyInteger', typename('var_myint'))
            self.assertEqual('MyString', typename('var_mystr'))
            self.assertEqual('MyDateTime', typename('var_mytime'))

            self.assertEqual('function', typeof('var_myint'))
            self.assertEqual('function', typeof('var_mystr'))
            self.assertEqual('function', typeof('var_mytime'))

    def testJavascriptWrapper(self):
        with STPyV8.JSContext() as ctxt:
            self.assertEqual(type(None), type(ctxt.eval("null")))
            self.assertEqual(type(None), type(ctxt.eval("undefined")))
            self.assertEqual(bool, type(ctxt.eval("true")))
            self.assertEqual(str, type(ctxt.eval("'test'")))
            self.assertEqual(int, type(ctxt.eval("123")))
            self.assertEqual(float, type(ctxt.eval("3.14")))
            self.assertEqual(datetime.datetime, type(ctxt.eval("new Date()")))
            self.assertEqual(STPyV8.JSArray, type(ctxt.eval("[1, 2, 3]")))
            self.assertEqual(STPyV8.JSFunction, type(ctxt.eval("(function() {})")))
            self.assertEqual(STPyV8.JSObject, type(ctxt.eval("new Object()")))

    def testPythonWrapper(self):
        with STPyV8.JSContext() as ctxt:
            typeof = ctxt.eval("(function type(value) { return typeof value; })")
            protoof = ctxt.eval("(function protoof(value) { return Object.prototype.toString.apply(value); })")

            self.assertEqual('[object Null]', protoof(None))
            self.assertEqual('boolean', typeof(True))
            self.assertEqual('number', typeof(123))
            self.assertEqual('number', typeof(3.14))
            self.assertEqual('string', typeof('test'))

            self.assertEqual('[object Date]', protoof(datetime.datetime.now()))
            self.assertEqual('[object Date]', protoof(datetime.date.today()))
            self.assertEqual('[object Date]', protoof(datetime.time()))

            def test():
                pass

            self.assertEqual('[object Function]', protoof(abs))
            self.assertEqual('[object Function]', protoof(test))
            self.assertEqual('[object Function]', protoof(self.testPythonWrapper))
            self.assertEqual('[object Function]', protoof(int))

    def testFunction(self):
        with STPyV8.JSContext() as ctxt:
            func = ctxt.eval("""
                (function ()
                {
                    function a()
                    {
                        return "abc";
                    }

                    return a();
                })
                """)

            self.assertEqual("abc", str(func()))
            self.assertTrue(func is not None)
            self.assertFalse(func is None)

            func = ctxt.eval("(function test() {})")

            self.assertEqual("test", func.name)
            self.assertEqual("", func.resname)
            self.assertEqual(0, func.linenum)
            self.assertEqual(14, func.colnum)

            # FIXME
            # Why the setter doesn't work?
            #
            # func.name = "hello"
            # self.assertEqual("hello", func.name)

            func.setName("hello")
            self.assertEqual("hello", func.name)

    def testCall(self):
        class Hello:
            def __call__(self, name):
                return "hello " + name

        class Global(STPyV8.JSClass):
            hello = Hello()

        with STPyV8.JSContext(Global()) as ctxt:
            self.assertEqual("hello world", ctxt.eval("hello('world')"))

    def testJSFunction(self):
        with STPyV8.JSContext() as ctxt:
            hello = ctxt.eval("(function (name) { return 'Hello ' + name; })")

            self.assertTrue(isinstance(hello, STPyV8.JSFunction))
            self.assertEqual("Hello world", hello('world'))
            self.assertEqual("Hello world", hello.invoke(['world']))

            obj = ctxt.eval("({ 'name': 'world', 'hello': function (name) { return 'Hello ' + name + ' from ' + this.name; }})")
            hello = obj.hello
            self.assertTrue(isinstance(hello, STPyV8.JSFunction))
            self.assertEqual("Hello world from world", hello('world'))

            tester = ctxt.eval("({ 'name': 'tester' })")
            self.assertEqual("Hello world from tester", hello.apply(tester, ['world']))
            self.assertEqual("Hello world from json", hello.apply({ 'name': 'json' }, ['world']))

    def testConstructor(self):
        with STPyV8.JSContext() as ctx:
            ctx.eval("""
                var Test = function() {
                    this.trySomething();
                };
                Test.prototype.trySomething = function() {
                    this.name = 'soirv8';
                };

                var Test2 = function(first_name, last_name) {
                    this.name = first_name + ' ' + last_name;
                };
                """)

            self.assertTrue(isinstance(ctx.locals.Test, STPyV8.JSFunction))

            test = STPyV8.JSObject.create(ctx.locals.Test)

            self.assertTrue(isinstance(ctx.locals.Test, STPyV8.JSObject))
            self.assertEqual("soirv8", test.name)

            test2 = STPyV8.JSObject.create(ctx.locals.Test2, ('John', 'Doe'))
            self.assertEqual("John Doe", test2.name)

            test3 = STPyV8.JSObject.create(ctx.locals.Test2, ('John', 'Doe'), { 'email': 'john.doe@randommail.com' })
            self.assertEqual("john.doe@randommail.com", test3.email)

    def testJSError(self):
        with STPyV8.JSContext() as ctxt:
            try:
                ctxt.eval('throw "test"')
                self.fail()
            except Exception: # pylint:disable=broad-except
                self.assertTrue(STPyV8.JSError, sys.exc_info()[0])

    def testErrorInfo(self):
        with STPyV8.JSContext():
            with STPyV8.JSEngine() as engine:
                try:
                    engine.compile("""
                        function hello()
                        {
                            throw Error("hello world");
                        }

                        hello();""", "test", 10, 10).run()
                    self.fail()
                except STPyV8.JSError as e:
                    self.assertTrue("JSError: Error: hello world ( test @ 14 : 28 )  ->" in str(e))
                    self.assertEqual("Error", e.name)
                    self.assertEqual("hello world", e.message)
                    self.assertEqual("test", e.scriptName)
                    self.assertEqual(14, e.lineNum)
                    self.assertEqual(96, e.startPos)
                    self.assertEqual(97, e.endPos)
                    self.assertEqual(28, e.startCol)
                    self.assertEqual(29, e.endCol)
                    self.assertEqual('throw Error("hello world");', e.sourceLine.strip())

                    self.assertEqual('Error: hello world\n' +
                                      '    at hello (test:14:35)\n' +
                                      '    at test:17:25', e.stackTrace)

    def testParseStack(self):
        self.assertEqual([
            ('Error', 'unknown source', None, None),
            ('test', 'native', None, None),
            ('<anonymous>', 'test0', 3, 5),
            ('f', 'test1', 2, 19),
            ('g', 'test2', 1, 15),
            (None, 'test3', 1, None),
            (None, 'test3', 1, 1),
        ], STPyV8.JSError.parse_stack("""Error: err
            at Error (unknown source)
            at test (native)
            at new <anonymous> (test0:3:5)
            at f (test1:2:19)
            at g (test2:1:15)
            at test3:1
            at test3:1:1"""))

    def testStackTrace(self):
        class Global(STPyV8.JSClass):
            def GetCurrentStackTrace(self, limit): # pylint:disable=no-self-use
                return STPyV8.JSStackTrace.GetCurrentStackTrace(limit, STPyV8.JSStackTrace.Options.Detailed)

        with STPyV8.JSContext(Global()) as ctxt:
            st = ctxt.eval("""
                function a()
                {
                    return GetCurrentStackTrace(4);
                }
                function b()
                {
                    return eval("a()");
                }
                function c()
                {
                    return new b();
                }
            c();""", "test")

            self.assertEqual(4, len(st))
            self.assertEqual("\tat a (test:4:28)\n\tat eval ((eval))\n\tat b (test:8:28)\n\tat c (test:12:28)\n", str(st))
            self.assertEqual("test.a (4:28)\n.eval (1:1) eval\ntest.b (8:28) constructor\ntest.c (12:28)",
                              "\n".join(["%s.%s (%d:%d)%s%s" % (
                                f.scriptName, f.funcName, f.lineNum, f.column,
                                ' eval' if f.isEval else '',
                                ' constructor' if f.isConstructor else '') for f in st]))

    def testPythonException(self):
        class Global(STPyV8.JSClass):
            def raiseException(self): # pylint:disable=no-self-use
                raise RuntimeError("Hello")

        with STPyV8.JSContext(Global()) as ctxt:
            ctxt.eval("""
                msg ="";
                try
                {
                    this.raiseException()
                }
                catch(e)
                {
                    msg += "catch " + e + ";";
                }
                finally
                {
                    msg += "finally";
                }""")
            self.assertEqual("catch Error: Hello;finally", str(ctxt.locals.msg))

    def testExceptionMapping(self):
        class TestException(Exception):
            pass

        class Global(STPyV8.JSClass):
            def raiseIndexError(self): # pylint:disable=no-self-use
                return [1, 2, 3][5]

            def raiseAttributeError(self): # pylint:disable=no-self-use
                None.hello()

            def raiseSyntaxError(self): # pylint:disable=no-self-use
                eval("???") # pylint:disable=eval-used

            def raiseTypeError(self): # pylint:disable=no-self-use
                int(sys)

            def raiseNotImplementedError(self): # pylint:disable=no-self-use
                raise NotImplementedError("Not supported")

            def raiseExceptions(self): # pylint:disable=no-self-use
                raise TestException()

        with STPyV8.JSContext(Global()) as ctxt:
            ctxt.eval("try { this.raiseIndexError(); } catch (e) { msg = e; }")

            self.assertEqual("RangeError: list index out of range", str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseAttributeError(); } catch (e) { msg = e; }")

            self.assertEqual("ReferenceError: 'NoneType' object has no attribute 'hello'", str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseSyntaxError(); } catch (e) { msg = e; }")

            self.assertEqual("SyntaxError: invalid syntax", str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseTypeError(); } catch (e) { msg = e; }")

            if sys.version_info.major >= 3:
                if sys.version_info.minor > 9:
                    self.assertEqual("TypeError: int() argument must be a string, a bytes-like object or a real number, not 'module'", str(ctxt.locals.msg))
                else:
                    self.assertEqual("TypeError: int() argument must be a string, a bytes-like object or a number, not 'module'", str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseNotImplementedError(); } catch (e) { msg = e; }")

            self.assertEqual("Error: Not supported", str(ctxt.locals.msg))

            self.assertRaises(TestException, ctxt.eval, "this.raiseExceptions();")

    def testArray(self):
        with STPyV8.JSContext() as ctxt:
            array = ctxt.eval("""
                var array = new Array();

                for (i=0; i<10; i++)
                {
                    array[i] = 10-i;
                }

                array;
                """)

            self.assertTrue(isinstance(array, STPyV8.JSArray))
            self.assertEqual(10, len(array))

            self.assertTrue(5 in array)
            self.assertFalse(15 in array)

            self.assertEqual(10, len(array))

            for i in range(10):
                self.assertEqual(10 - i, array[i])

            array[5] = 0

            self.assertEqual(0, array[5])

            del array[5]

            self.assertEqual(None, array[5])

            # array         [10, 9, 8, 7, 6, None, 4, 3, 2, 1]
            # array[4:7]                  4^^^^^^^^^7
            # array[-3:-1]                         -3^^^^^^-1
            # array[0:0]    []

            self.assertEqual([6, None, 4], array[4:7])
            self.assertEqual([3, 2], array[-3:-1])
            self.assertEqual([], array[0:0])

            self.assertEqual([10, 9, 8, 7, 6, None, 4, 3, 2, 1], list(array))

            array[1:3] = [9, 9]

            self.assertEqual([10, 9, 9, 7, 6, None, 4, 3, 2, 1], list(array))

            array[5:7] = [8, 8]

            self.assertEqual([10, 9, 9, 7, 6, 8, 8, 3, 2, 1], list(array))

            del array[1]

            self.assertEqual([10, None, 9, 7, 6, 8, 8, 3, 2, 1], list(array))

            ctxt.locals.array1 = STPyV8.JSArray(5)
            ctxt.locals.array2 = STPyV8.JSArray([1, 2, 3, 4, 5])

            for i in range(len(ctxt.locals.array2)):
                ctxt.locals.array1[i] = ctxt.locals.array2[i] * 10

            ctxt.eval("""
                var sum = 0;

                for (i=0; i<array1.length; i++)
                    sum += array1[i]

                for (i=0; i<array2.length; i++)
                    sum += array2[i]
                """)

            self.assertEqual(165, ctxt.locals.sum)

            ctxt.locals.array3 = [1, 2, 3, 4, 5]
            self.assertTrue(ctxt.eval('array3[1] === 2'))
            self.assertTrue(ctxt.eval('array3[9] === undefined'))

            args = [
                ["a = Array(7); for(i=0; i<a.length; i++) a[i] = i; a[3] = undefined; a[a.length-1]; a", "0,1,2,,4,5,6", [0, 1, 2, None, 4, 5, 6]],
                ["a = Array(7); for(i=0; i<a.length - 1; i++) a[i] = i; a[a.length-1]; a", "0,1,2,3,4,5,", [0, 1, 2, 3, 4, 5, None]],
                ["a = Array(7); for(i=1; i<a.length; i++) a[i] = i; a[a.length-1]; a", ",1,2,3,4,5,6", [None, 1, 2, 3, 4, 5, 6]]
            ]

            for arg in args:
                array = ctxt.eval(arg[0])

                self.assertEqual(arg[1], str(array))
                self.assertEqual(arg[2], [array[i] for i in range(len(array))])

            self.assertEqual(3, ctxt.eval("(function (arr) { return arr.length; })")(STPyV8.JSArray([1, 2, 3])))
            self.assertEqual(2, ctxt.eval("(function (arr, idx) { return arr[idx]; })")(STPyV8.JSArray([1, 2, 3]), 1))
            self.assertEqual('[object Array]', ctxt.eval("(function (arr) { return Object.prototype.toString.call(arr); })")(STPyV8.JSArray([1, 2, 3])))
            self.assertEqual('[object Array]', ctxt.eval("(function (arr) { return Object.prototype.toString.call(arr); })")(STPyV8.JSArray((1, 2, 3))))
            self.assertEqual('[object Array]', ctxt.eval("(function (arr) { return Object.prototype.toString.call(arr); })")(STPyV8.JSArray(list(range(3)))))

    def testArraySlices(self):
        with STPyV8.JSContext() as ctxt:
            array = ctxt.eval("""
                var array = new Array();
                array;
            """)

            array[2:4] = [42, 24]
            # array         [None, None, 42, 24]
            self.assertEqual(len(array), 4)
            self.assertEqual(ctxt.eval('array[0]'), None)
            self.assertEqual(ctxt.eval('array[1]'), None)
            self.assertEqual(ctxt.eval('array[2]'), 42)
            self.assertEqual(ctxt.eval('array[3]'), 24)

            array[2:4] = [1, 2]
            # array         [None, None, 1, 2]
            self.assertEqual(len(array), 4)
            self.assertEqual(ctxt.eval('array[0]'), None)
            self.assertEqual(ctxt.eval('array[1]'), None)
            self.assertEqual(ctxt.eval('array[2]'), 1)
            self.assertEqual(ctxt.eval('array[3]'), 2)

            array[0:4:2] = [7, 8]
            # array         [7, None, 8, 2]
            self.assertEqual(len(array), 4)
            self.assertEqual(ctxt.eval('array[0]'), 7)
            self.assertEqual(ctxt.eval('array[1]'), None)
            self.assertEqual(ctxt.eval('array[2]'), 8)
            self.assertEqual(ctxt.eval('array[3]'), 2)

            array[1:4] = [10]
            # array         [7, 10, None, None]
            self.assertEqual(len(array), 4)
            self.assertEqual(ctxt.eval('array[0]'), 7)
            self.assertEqual(ctxt.eval('array[1]'), 10)
            self.assertEqual(ctxt.eval('array[2]'), None)
            self.assertEqual(ctxt.eval('array[3]'), None)

            array[0:7] = [0, 1, 2]
            # array         [0, 1, 2, None, None, None, None]
            self.assertEqual(len(array), 7)
            self.assertEqual(ctxt.eval('array[0]'), 0)
            self.assertEqual(ctxt.eval('array[1]'), 1)
            self.assertEqual(ctxt.eval('array[2]'), 2)
            self.assertEqual(ctxt.eval('array[3]'), None)
            self.assertEqual(ctxt.eval('array[4]'), None)
            self.assertEqual(ctxt.eval('array[5]'), None)
            self.assertEqual(ctxt.eval('array[6]'), None)

            array[0:7] = [0, 1, 2, 3, 4, 5, 6]
            # array         [0, 1, 2, 3, 4, 5, 6]
            self.assertEqual(len(array), 7)
            self.assertEqual(ctxt.eval('array[0]'), 0)
            self.assertEqual(ctxt.eval('array[1]'), 1)
            self.assertEqual(ctxt.eval('array[2]'), 2)
            self.assertEqual(ctxt.eval('array[3]'), 3)
            self.assertEqual(ctxt.eval('array[4]'), 4)
            self.assertEqual(ctxt.eval('array[5]'), 5)
            self.assertEqual(ctxt.eval('array[6]'), 6)

            del array[0:2]
            # array         [None, None, 2, 3, 4, 5, 6]
            self.assertEqual(len(array), 7)
            self.assertEqual(ctxt.eval('array[0]'), None)
            self.assertEqual(ctxt.eval('array[1]'), None)
            self.assertEqual(ctxt.eval('array[2]'), 2)
            self.assertEqual(ctxt.eval('array[3]'), 3)
            self.assertEqual(ctxt.eval('array[4]'), 4)
            self.assertEqual(ctxt.eval('array[5]'), 5)
            self.assertEqual(ctxt.eval('array[6]'), 6)

            del array[3:7:2]
            # array         [None, None, 2, None, 4, None, 6]
            self.assertEqual(len(array), 7)
            self.assertEqual(ctxt.eval('array[0]'), None)
            self.assertEqual(ctxt.eval('array[1]'), None)
            self.assertEqual(ctxt.eval('array[2]'), 2)
            self.assertEqual(ctxt.eval('array[3]'), None)
            self.assertEqual(ctxt.eval('array[4]'), 4)
            self.assertEqual(ctxt.eval('array[5]'), None)
            self.assertEqual(ctxt.eval('array[6]'), 6)

    def testMultiDimArray(self):
        with STPyV8.JSContext() as ctxt:
            ret = ctxt.eval("""
                ({
                    'test': function(){
                        return  [
                            [ 1, 'abla' ],
                            [ 2, 'ajkss' ],
                        ]
                    }
                })
                """).test()

            self.assertEqual([[1, 'abla'], [2, 'ajkss']], convert(ret))

    def testLazyConstructor(self):
        class Globals(STPyV8.JSClass):
            def __init__(self):
                self.array=STPyV8.JSArray([1,2,3])

        with STPyV8.JSContext(Globals()) as ctxt:
            self.assertEqual(2, ctxt.eval("""array[1]"""))

    def testForEach(self):
        class NamedClass:
            foo = 1 # pylint:disable=disallowed-name

            def __init__(self):
                self.bar = 2 # pylint:disable=disallowed-name

            @property
            def foobar(self):
                return self.foo + self.bar

        def gen(x):
            for i in range(x):
                yield i

        with STPyV8.JSContext() as ctxt:
            func = ctxt.eval("""(function (k) {
                var result = [];
                for (var prop in k) {
                  result.push(prop);
                }
                return result;
            })""")

            self.assertTrue(set(["bar", "foo", "foobar"]).issubset(set(func(NamedClass()))))
            self.assertEqual(["0", "1", "2"], list(func([1, 2, 3])))
            self.assertEqual(["0", "1", "2"], list(func((1, 2, 3))))
            self.assertEqual(["1", "2", "3"], list(func({'1' : 1, '2' : 2, '3' : 3})))

            self.assertEqual(["0", "1", "2"], list(func(list(gen(3)))))

    def testDict(self):
        with STPyV8.JSContext() as ctxt:
            obj = ctxt.eval("var r = { 'a' : 1, 'b' : 2 }; r")

            self.assertEqual(1, obj.a)
            self.assertEqual(2, obj.b)

            self.assertEqual({ 'a' : 1, 'b' : 2 }, dict(obj))

            self.assertEqual({ 'a': 1,
                               'b': [1, 2, 3],
                               'c': { 'str' : 'goofy',
                                      'float' : 1.234,
                                      'obj' : { 'name': 'john doe' }},
                               'd': True,
                               'e': None },
                             convert(ctxt.eval("""var x =
                             { a: 1,
                               b: [1, 2, 3],
                               c: { str: 'goofy',
                                    float: 1.234,
                                    obj: { name: 'john doe' }},
                               d: true,
                               e: null }; x""")))

    def testDate(self):
        with STPyV8.JSContext() as ctxt:
            now1 = ctxt.eval("new Date();")

            self.assertTrue(now1)

            now2 = datetime.datetime.now()

            delta = now2 - now1 if now2 > now1 else now1 - now2

            self.assertTrue(delta < datetime.timedelta(seconds = 1))

            func = ctxt.eval("(function (d) { return d.toString(); })")

            now = datetime.datetime.now()

            self.assertTrue(str(func(now)).startswith(now.strftime("%a %b %d %Y %H:%M:%S")))

            ctxt.eval("function identity(x) { return x; }")

            now3 = now2.replace(microsecond = 123000)
            self.assertEqual(now3, ctxt.locals.identity(now3))

    def testUnicode(self):
        with STPyV8.JSContext() as ctxt:
            self.assertEqual("人", ctxt.eval("\"人\""))
            self.assertEqual("é", ctxt.eval("\"é\""))

            func = ctxt.eval("(function (msg) { return msg.length; })")

            self.assertEqual(2, func("测试"))

    def testClassicStyleObject(self):
        class FileSystemWrapper:
            @property
            def cwd(self):
                return os.getcwd()

        class Global:
            @property
            def fs(self):
                return FileSystemWrapper()

        with STPyV8.JSContext(Global()) as ctxt:
            self.assertEqual(os.getcwd(), ctxt.eval("fs.cwd"))

    def testRefCount(self):
        count = sys.getrefcount(None)

        class Global(STPyV8.JSClass):
            pass

        g = Global()
        g_refs = sys.getrefcount(g)

        with STPyV8.JSContext(g) as ctxt:
            ctxt.eval("""
                var none = null;
            """)

            self.assertEqual(count+1, sys.getrefcount(None))

            ctxt.eval("""
                var none = null;
            """)

            self.assertEqual(count+1, sys.getrefcount(None))

            del ctxt

        self.assertEqual(g_refs, sys.getrefcount(g))

    def testProperty(self):
        class Global(STPyV8.JSClass):
            def __init__(self, name):
                self._name = name

            def getname(self):
                return self._name

            def setname(self, name):
                self._name = name

            def delname(self):
                self._name = 'deleted'

            name = property(getname, setname, delname)

        g = Global('world')

        with STPyV8.JSContext(g) as ctxt:
            self.assertEqual('world', ctxt.eval("name"))
            self.assertEqual('foobar', ctxt.eval("this.name = 'foobar';"))
            self.assertEqual('foobar', ctxt.eval("name"))
            self.assertTrue(ctxt.eval("delete name"))

            # FIXME replace the global object with Python object
            #
            # self.assertEqual('deleted', ctxt.eval("name"))
            # ctxt.eval("__defineGetter__('name', function() { return 'fixed'; });")
            # self.assertEqual('fixed', ctxt.eval("name"))

        with STPyV8.JSContext() as ctxt:
            self.assertEqual('world', ctxt.eval("name = 'world';"))
            self.assertTrue(ctxt.eval("delete name;"))
            self.assertRaises(ReferenceError, ctxt.eval, 'name')

            ctxt.eval("""
                let obj = {};
                obj.__defineGetter__('name', function() {
                    return 'test';
                });
            """)
            self.assertEqual('test', ctxt.eval("obj.name"))

    def testGetterAndSetter(self):
        class Global(STPyV8.JSClass):
            def __init__(self, testval):
                self.testval = testval

        with STPyV8.JSContext(Global("Test Value A")) as ctxt:
            self.assertEqual("Test Value A", ctxt.locals.testval)
            ctxt.eval("""
                this.__defineGetter__("test", function() {
                    return this.testval;
                });
                this.__defineSetter__("test", function(val) {
                    this.testval = val;
                });
            """)

            self.assertEqual("Test Value A",  ctxt.locals.test)

            ctxt.eval("test = 'Test Value B';")

            self.assertEqual("Test Value B",  ctxt.locals.test)

    def testReferenceCount(self):
        class Hello:
            def say(self):
                pass

            def __del__(self):
                owner.deleted = True # pylint:disable=undefined-variable

        def test():
            with STPyV8.JSContext() as ctxt:
                fn = ctxt.eval("(function (obj) { obj.say(); })")

                obj = Hello()

                self.assertEqual(2, sys.getrefcount(obj))

                fn(obj)

                self.assertEqual(4, sys.getrefcount(obj))

                del obj

        test()

    def testNullInString(self):
        with STPyV8.JSContext() as ctxt:
            fn = ctxt.eval("(function (s) { return s; })")

            self.assertEqual("hello \0 world", fn("hello \0 world"))

    def testLivingObjectCache(self):
        class Global(STPyV8.JSClass):
            i = 1
            b = True
            o = object()

        with STPyV8.JSContext(Global()) as ctxt:
            self.assertTrue(ctxt.eval("i == i"))
            self.assertTrue(ctxt.eval("b == b"))
            self.assertTrue(ctxt.eval("o == o"))

    def testNamedSetter(self):
        class Obj(STPyV8.JSClass):
            @property
            def p(self):
                return self._p

            @p.setter
            def p(self, value):
                self._p = value

        class Global(STPyV8.JSClass):
            def __init__(self):
                self.obj = Obj()
                self.d = {}
                self.p = None

        with STPyV8.JSContext(Global()) as ctxt:
            ctxt.eval("""
            x = obj;
            x.y = 10;
            x.p = 10;
            d.y = 10;
            """)
            self.assertEqual(10, ctxt.eval("obj.y"))
            self.assertEqual(10, ctxt.eval("obj.p"))
            self.assertEqual(10, ctxt.locals.d['y'])

    def testWatch(self):
        class Obj(STPyV8.JSClass):
            def __init__(self):
                self.p = 1

        class Global(STPyV8.JSClass):
            def __init__(self):
                self.o = Obj()

        with STPyV8.JSContext(Global()) as ctxt:
            ctxt.eval("""
            o.watch("p", function (id, oldval, newval) {
                return oldval + newval;
            });
            """)

            self.assertEqual(1, ctxt.eval("o.p"))

            ctxt.eval("o.p = 2;")

            self.assertEqual(3, ctxt.eval("o.p"))

            ctxt.eval("delete o.p;")

            self.assertEqual(None, ctxt.eval("o.p"))

            ctxt.eval("o.p = 2;")

            self.assertEqual(2, ctxt.eval("o.p"))

            ctxt.eval("o.unwatch('p');")

            ctxt.eval("o.p = 1;")

            self.assertEqual(1, ctxt.eval("o.p"))

    def testReferenceError(self):
        class Global(STPyV8.JSClass):
            def __init__(self):
                self.s = self

        with STPyV8.JSContext(Global()) as ctxt:
            self.assertRaises(ReferenceError, ctxt.eval, 'x')

            self.assertTrue(ctxt.eval("typeof(x) === 'undefined'"))

            self.assertTrue(ctxt.eval("typeof(String) === 'function'"))

            self.assertTrue(ctxt.eval("typeof(s.String) === 'undefined'"))

            self.assertTrue(ctxt.eval("typeof(s.z) === 'undefined'"))

    def testRaiseExceptionInGetter(self):
        class Document(STPyV8.JSClass):
            def __getattr__(self, name):
                if name == 'y':
                    raise TypeError()

                return STPyV8.JSClass.__getattr__(self, name)

        class Global(STPyV8.JSClass):
            def __init__(self):
                self.document = Document()

        with STPyV8.JSContext(Global()) as ctxt:
            self.assertEqual(None, ctxt.eval('document.x'))
            self.assertRaises(TypeError, ctxt.eval, 'document.y')

    def testUndefined(self):
        class Global(STPyV8.JSClass):
            def returnNull(self): # pylint:disable=no-self-use
                return STPyV8.JSNull()

            def returnUndefined(self): # pylint:disable=no-self-use
                return STPyV8.JSUndefined()

            def returnNone(self): # pylint:disable=no-self-use
                return None

        with STPyV8.JSContext(Global()) as ctxt:
            self.assertFalse(bool(STPyV8.JSNull()))
            self.assertFalse(bool(STPyV8.JSUndefined()))

            self.assertEqual("null", str(STPyV8.JSNull()))
            self.assertEqual("undefined", str(STPyV8.JSUndefined()))

            self.assertTrue(ctxt.eval('null == returnNull()'))
            self.assertTrue(ctxt.eval('undefined == returnUndefined()'))
            self.assertTrue(ctxt.eval('null == returnNone()'))
