#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
import unittest
import logging

from datetime import *

import SoirV8

is_py3k = sys.version_info[0] > 2

if is_py3k:
    def toNativeString(s):
        return s
    def toUnicodeString(s):
        return s
else:
    def toNativeString(s, encoding = 'utf-8'):
        return s.encode(encoding) if isinstance(s, unicode) else s

    def toUnicodeString(s, encoding = 'utf-8'):
        return s if isinstance(s, unicode) else unicode(s, encoding)


def convert(obj):
    if type(obj) == SoirV8.JSArray:
        return [convert(v) for v in obj]

    if type(obj) == SoirV8.JSObject:
        return dict([[str(k), convert(obj.__getattr__(str(k)))] for k in (obj.__dir__() if is_py3k else obj.__members__)])

    return obj


class TestWrapper(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.platform = SoirV8.JSPlatform()
        self.platform.init()

        self.isolate = SoirV8.JSIsolate()
        self.isolate.enter()  #TODO remove?

    def testObject(self):
        with SoirV8.JSContext() as ctxt:
            o = ctxt.eval("new Object()")

            self.assertTrue(hash(o) > 0)

            o1 = o.clone()

            self.assertEqual(hash(o1), hash(o))
            self.assertTrue(o != o1)

        self.assertRaises(UnboundLocalError, o.clone)

    def testAutoConverter(self):
        with SoirV8.JSContext() as ctxt:
            ctxt.eval("""
                var_i = 1;
                var_f = 1.0;
                var_s = "test";
                var_b = true;
                var_s_obj = new String("test");
                var_b_obj = new Boolean(true);
                var_f_obj = new Number(1.5);
            """)

            vars = ctxt.locals

            var_i = vars.var_i

            self.assertTrue(var_i)
            self.assertEqual(1, int(var_i))

            var_f = vars.var_f

            self.assertTrue(var_f)
            self.assertEqual(1.0, float(vars.var_f))

            var_s = vars.var_s
            self.assertTrue(var_s)
            self.assertEqual("test", str(vars.var_s))

            var_b = vars.var_b
            self.assertTrue(var_b)
            self.assertTrue(bool(var_b))

            self.assertEqual("test", vars.var_s_obj)
            self.assertTrue(vars.var_b_obj)
            self.assertEqual(1.5, vars.var_f_obj)

            attrs = dir(ctxt.locals)

            self.assertTrue(attrs)
            self.assertTrue("var_i" in attrs)
            self.assertTrue("var_f" in attrs)
            self.assertTrue("var_s" in attrs)
            self.assertTrue("var_b" in attrs)
            self.assertTrue("var_s_obj" in attrs)
            self.assertTrue("var_b_obj" in attrs)
            self.assertTrue("var_f_obj" in attrs)

    def _testExactConverter(self):
        class MyInteger(int, SoirV8.JSClass):
            pass

        class MyString(str, SoirV8.JSClass):
            pass

        class MyUnicode(unicode, SoirV8.JSClass):
            pass

        class MyDateTime(time, SoirV8.JSClass):
            pass

        class Global(SoirV8.JSClass):
            var_bool = True
            var_int = 1
            var_float = 1.0
            var_str = 'str'
            var_unicode = u'unicode'
            var_datetime = datetime.now()
            var_date = date.today()
            var_time = time()

            var_myint = MyInteger()
            var_mystr = MyString('mystr')
            var_myunicode = MyUnicode('myunicode')
            var_mytime = MyDateTime()

        with SoirV8.JSContext(Global()) as ctxt:
            typename = ctxt.eval("(function (name) { return this[name].constructor.name; })")
            typeof = ctxt.eval("(function (name) { return typeof(this[name]); })")

            self.assertEqual('Boolean', typename('var_bool'))
            self.assertEqual('Number', typename('var_int'))
            self.assertEqual('Number', typename('var_float'))
            self.assertEqual('String', typename('var_str'))
            self.assertEqual('String', typename('var_unicode'))
            self.assertEqual('Date', typename('var_datetime'))
            self.assertEqual('Date', typename('var_date'))
            self.assertEqual('Date', typename('var_time'))

            self.assertEqual('MyInteger', typename('var_myint'))
            self.assertEqual('MyString', typename('var_mystr'))
            self.assertEqual('MyUnicode', typename('var_myunicode'))
            self.assertEqual('MyDateTime', typename('var_mytime'))

            self.assertEqual('object', typeof('var_myint'))
            self.assertEqual('object', typeof('var_mystr'))
            self.assertEqual('object', typeof('var_myunicode'))
            self.assertEqual('object', typeof('var_mytime'))

    def testJavascriptWrapper(self):
        with SoirV8.JSContext() as ctxt:
            self.assertEqual(type(None), type(ctxt.eval("null")))
            self.assertEqual(type(None), type(ctxt.eval("undefined")))
            self.assertEqual(bool, type(ctxt.eval("true")))
            self.assertEqual(str, type(ctxt.eval("'test'")))
            self.assertEqual(int, type(ctxt.eval("123")))
            self.assertEqual(float, type(ctxt.eval("3.14")))
            self.assertEqual(datetime, type(ctxt.eval("new Date()")))
            self.assertEqual(SoirV8.JSArray, type(ctxt.eval("[1, 2, 3]")))
            self.assertEqual(SoirV8.JSFunction, type(ctxt.eval("(function() {})")))
            self.assertEqual(SoirV8.JSObject, type(ctxt.eval("new Object()")))

    def testPythonWrapper(self):
        with SoirV8.JSContext() as ctxt:
            typeof = ctxt.eval("(function type(value) { return typeof value; })")
            protoof = ctxt.eval("(function protoof(value) { return Object.prototype.toString.apply(value); })")

            self.assertEqual('[object Null]', protoof(None))
            self.assertEqual('boolean', typeof(True))
            self.assertEqual('number', typeof(123))
            self.assertEqual('number', typeof(3.14))
            self.assertEqual('string', typeof('test'))
            self.assertEqual('string', typeof(u'test'))

            self.assertEqual('[object Date]', protoof(datetime.now()))
            self.assertEqual('[object Date]', protoof(date.today()))
            self.assertEqual('[object Date]', protoof(time()))

            def test():
                pass

            self.assertEqual('[object Function]', protoof(abs))
            self.assertEqual('[object Function]', protoof(test))
            self.assertEqual('[object Function]', protoof(self.testPythonWrapper))
            self.assertEqual('[object Function]', protoof(int))

    def testFunction(self):
        with SoirV8.JSContext() as ctxt:
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
            self.assertTrue(func != None)
            self.assertFalse(func == None)

            func = ctxt.eval("(function test() {})")

            self.assertEqual("test", func.name)
            self.assertEqual("", func.resname)
            self.assertEqual(0, func.linenum)
            self.assertEqual(14, func.colnum)
            self.assertEqual(0, func.lineoff)
            self.assertEqual(0, func.coloff)

            #TODO fix me, why the setter doesn't work?
            # func.name = "hello"
            # self.assertEqual("hello", func.name)

            func.setName("hello")
            self.assertEqual("hello", func.name)

    def testCall(self):
        class Hello(object):
            def __call__(self, name):
                return "hello " + name

        class Global(SoirV8.JSClass):
            hello = Hello()

        with SoirV8.JSContext(Global()) as ctxt:
            self.assertEqual("hello world", ctxt.eval("hello('world')"))

    def testJSFunction(self):
        with SoirV8.JSContext() as ctxt:
            hello = ctxt.eval("(function (name) { return 'Hello ' + name; })")

            self.assertTrue(isinstance(hello, SoirV8.JSFunction))
            self.assertEqual("Hello world", hello('world'))
            self.assertEqual("Hello world", hello.invoke(['world']))

            obj = ctxt.eval("({ 'name': 'world', 'hello': function (name) { return 'Hello ' + name + ' from ' + this.name; }})")
            hello = obj.hello
            self.assertTrue(isinstance(hello, SoirV8.JSFunction))
            self.assertEqual("Hello world from world", hello('world'))

            tester = ctxt.eval("({ 'name': 'tester' })")
            self.assertEqual("Hello world from tester", hello.apply(tester, ['world']))
            self.assertEqual("Hello world from json", hello.apply({ 'name': 'json' }, ['world']))

    def testConstructor(self):
        with SoirV8.JSContext() as ctx:
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

            self.assertTrue(isinstance(ctx.locals.Test, SoirV8.JSFunction))

            test = SoirV8.JSObject.create(ctx.locals.Test)

            self.assertTrue(isinstance(ctx.locals.Test, SoirV8.JSObject))
            self.assertEqual("soirv8", test.name);

            test2 = SoirV8.JSObject.create(ctx.locals.Test2, ('John', 'Doe'))

            self.assertEqual("John Doe", test2.name);

            test3 = SoirV8.JSObject.create(ctx.locals.Test2, ('John', 'Doe'), { 'email': 'john.doe@randommail.com' })

            self.assertEqual("john.doe@randommail.com", test3.email);

    def testJSError(self):
        with SoirV8.JSContext() as ctxt:
            try:
                ctxt.eval('throw "test"')
                self.fail()
            except:
                self.assertTrue(SoirV8.JSError, sys.exc_info()[0])

    def _testErrorInfo(self):
        with SoirV8.JSContext() as ctxt:
            with SoirV8.JSEngine() as engine:
                try:
                    engine.compile("""
                        function hello()
                        {
                            throw Error("hello world");
                        }

                        hello();""", "test", 10, 10).run()
                    self.fail()
                except SoirV8.JSError as e:
                    self.assertTrue(str(e).startswith('SoirV8.JSError: Error: hello world ( test @ 14 : 34 )  ->'))
                    self.assertEqual("Error", e.name)
                    self.assertEqual("hello world", e.message)
                    self.assertEqual("test", e.scriptName)
                    self.assertEqual(14, e.lineNum)
                    self.assertEqual(102, e.startPos)
                    self.assertEqual(103, e.endPos)
                    self.assertEqual(34, e.startCol)
                    self.assertEqual(35, e.endCol)
                    self.assertEqual('throw Error("hello world");', e.sourceLine.strip())
                    self.assertEqual('Error: hello world\n' +
                                     '    at Error (native)\n' +
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
        ], SoirV8.JSError.parse_stack("""Error: err
            at Error (unknown source)
            at test (native)
            at new <anonymous> (test0:3:5)
            at f (test1:2:19)
            at g (test2:1:15)
            at test3:1
            at test3:1:1"""))

    def _testStackTrace(self):
        class Global(SoirV8.JSClass):
            def GetCurrentStackTrace(self, limit):
                return SoirV8.JSStackTrace.GetCurrentStackTrace(4, SoirV8.JSStackTrace.Options.Detailed)

        with SoirV8.JSContext(Global()) as ctxt:
            st = ctxt.eval("""
                function a()
                {
                    return GetCurrentStackTrace(10);
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
            self.assertEqual("\tat a (test:4:28)\n\tat (eval)\n\tat b (test:8:28)\n\tat c (test:12:28)\n", str(st))
            self.assertEqual("test.a (4:28)\n. (1:1) eval\ntest.b (8:28) constructor\ntest.c (12:28)",
                              "\n".join(["%s.%s (%d:%d)%s%s" % (
                                f.scriptName, f.funcName, f.lineNum, f.column,
                                ' eval' if f.isEval else '',
                                ' constructor' if f.isConstructor else '') for f in st]))

    def testPythonException(self):
        class Global(SoirV8.JSClass):
            def raiseException(self):
                raise RuntimeError("Hello")

        with SoirV8.JSContext(Global()) as ctxt:
            r = ctxt.eval("""
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

    def _testExceptionMapping(self):
        class TestException(Exception):
            pass

        class Global(SoirV8.JSClass):
            def raiseIndexError(self):
                return [1, 2, 3][5]

            def raiseAttributeError(self):
                None.hello()

            def raiseSyntaxError(self):
                eval("???")

            def raiseTypeError(self):
                int(sys)

            def raiseNotImplementedError(self):
                raise NotImplementedError("Not support")

            def raiseExceptions(self):
                raise TestException()

        with SoirV8.JSContext(Global()) as ctxt:
            ctxt.eval("try { this.raiseIndexError(); } catch (e) { msg = e; }")

            self.assertEqual("RangeError: list index out of range", str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseAttributeError(); } catch (e) { msg = e; }")

            self.assertEqual("ReferenceError: 'NoneType' object has no attribute 'hello'", str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseSyntaxError(); } catch (e) { msg = e; }")

            self.assertEqual("SyntaxError: invalid syntax", str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseTypeError(); } catch (e) { msg = e; }")

            self.assertEqual("TypeError: int() argument must be a string or a number, not 'module'", str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseNotImplementedError(); } catch (e) { msg = e; }")

            self.assertEqual("Error: Not support", str(ctxt.locals.msg))

            self.assertRaises(TestException, ctxt.eval, "this.raiseExceptions();")

    def testArray(self):
        with SoirV8.JSContext() as ctxt:
            array = ctxt.eval("""
                var array = new Array();

                for (i=0; i<10; i++)
                {
                    array[i] = 10-i;
                }

                array;
                """)

            self.assertTrue(isinstance(array, SoirV8.JSArray))
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

            ctxt.locals.array1 = SoirV8.JSArray(5)
            ctxt.locals.array2 = SoirV8.JSArray([1, 2, 3, 4, 5])

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

            self.assertEqual(3, ctxt.eval("(function (arr) { return arr.length; })")(SoirV8.JSArray([1, 2, 3])))
            self.assertEqual(2, ctxt.eval("(function (arr, idx) { return arr[idx]; })")(SoirV8.JSArray([1, 2, 3]), 1))
            self.assertEqual('[object Array]', ctxt.eval("(function (arr) { return Object.prototype.toString.call(arr); })")(SoirV8.JSArray([1, 2, 3])))
            self.assertEqual('[object Array]', ctxt.eval("(function (arr) { return Object.prototype.toString.call(arr); })")(SoirV8.JSArray((1, 2, 3))))
            self.assertEqual('[object Array]', ctxt.eval("(function (arr) { return Object.prototype.toString.call(arr); })")(SoirV8.JSArray(range(3))))

            [x for x in SoirV8.JSArray([1,2,3])]

    def testMultiDimArray(self):
        with SoirV8.JSContext() as ctxt:
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
        class Globals(SoirV8.JSClass):
            def __init__(self):
                self.array=SoirV8.JSArray([1,2,3])

        with SoirV8.JSContext(Globals()) as ctxt:
            self.assertEqual(2, ctxt.eval("""array[1]"""))

    def _testForEach(self):
        class NamedClass(object):
            foo = 1

            def __init__(self):
                self.bar = 2

            @property
            def foobar(self):
                return self.foo + self.bar

        def gen(x):
            for i in range(x):
                yield i

        with SoirV8.JSContext() as ctxt:
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
            self.assertEqual(["1", "2", "3"], list(func({1:1, 2:2, 3:3})))

            self.assertEqual(["0", "1", "2"], list(func(gen(3))))

    def testDict(self):
        with SoirV8.JSContext() as ctxt:
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
        with SoirV8.JSContext() as ctxt:
            now1 = ctxt.eval("new Date();")

            self.assertTrue(now1)

            now2 = datetime.now()

            delta = now2 - now1 if now2 > now1 else now1 - now2

            self.assertTrue(delta < timedelta(seconds=1))

            func = ctxt.eval("(function (d) { return d.toString(); })")

            now = datetime.now()

            self.assertTrue(str(func(now)).startswith(now.strftime("%a %b %d %Y %H:%M:%S")))

            ctxt.eval("function identity(x) { return x; }")
            # SoirV8.JS only has millisecond resolution, so cut it off there
            now3 = now2.replace(microsecond=123000)
            self.assertEqual(now3, ctxt.locals.identity(now3))

    def testUnicode(self):
        with SoirV8.JSContext() as ctxt:
            self.assertEqual(u"人", toUnicodeString(ctxt.eval(u"\"人\"")))
            self.assertEqual(u"é", toUnicodeString(ctxt.eval(u"\"é\"")))

            func = ctxt.eval("(function (msg) { return msg.length; })")

            self.assertEqual(2, func(u"测试"))

    def testClassicStyleObject(self):
        class FileSystemWrapper:
            @property
            def cwd(self):
                return os.getcwd()

        class Global:
            @property
            def fs(self):
                return FileSystemWrapper()

        with SoirV8.JSContext(Global()) as ctxt:
            self.assertEqual(os.getcwd(), ctxt.eval("fs.cwd"))

    def testRefCount(self):
        count = sys.getrefcount(None)

        class Global(SoirV8.JSClass):
            pass

        g = Global()
        g_refs = sys.getrefcount(g)

        with SoirV8.JSContext(g) as ctxt:
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
        class Global(SoirV8.JSClass):
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

        with SoirV8.JSContext(g) as ctxt:
            self.assertEqual('world', ctxt.eval("name"))
            self.assertEqual('flier', ctxt.eval("this.name = 'flier';"))
            self.assertEqual('flier', ctxt.eval("name"))
            self.assertTrue(ctxt.eval("delete name"))
            ###
            # FIXME replace the global object with Python object
            #
            #self.assertEqual('deleted', ctxt.eval("name"))
            #ctxt.eval("__defineGetter__('name', function() { return 'fixed'; });")
            #self.assertEqual('fixed', ctxt.eval("name"))

    def testGetterAndSetter(self):
        class Global(SoirV8.JSClass):
           def __init__(self, testval):
               self.testval = testval

        with SoirV8.JSContext(Global("Test Value A")) as ctxt:
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

    def _testDestructor(self):
        import gc

        owner = self
        owner.deleted = False

        class Hello(object):
            def say(self):
                pass

            def __del__(self):
                owner.deleted = True

        def test():
            with SoirV8.JSContext() as ctxt:
                fn = ctxt.eval("(function (obj) { obj.say(); })")

                obj = Hello()

                self.assertEqual(2, sys.getrefcount(obj))

                fn(obj)

                self.assertEqual(4, sys.getrefcount(obj))

                del obj

        test()

        self.assertFalse(owner.deleted)

        SoirV8.JSEngine.collect()
        gc.collect()

        self.assertTrue(owner.deleted)

    def testNullInString(self):
        with SoirV8.JSContext() as ctxt:
            fn = ctxt.eval("(function (s) { return s; })")

            self.assertEqual("hello \0 world", fn("hello \0 world"))

    def _testLivingObjectCache(self):
        class Global(SoirV8.JSClass):
            i = 1
            b = True
            o = object()

        with SoirV8.JSContext(Global()) as ctxt:
            self.assertTrue(ctxt.eval("i == i"))
            self.assertTrue(ctxt.eval("b == b"))
            self.assertTrue(ctxt.eval("o == o"))

    def testNamedSetter(self):
        class Obj(SoirV8.JSClass):
            @property
            def p(self):
                return self._p

            @p.setter
            def p(self, value):
                self._p = value

        class Global(SoirV8.JSClass):
            def __init__(self):
                self.obj = Obj()
                self.d = {}
                self.p = None

        with SoirV8.JSContext(Global()) as ctxt:
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
        class Obj(SoirV8.JSClass):
            def __init__(self):
                self.p = 1

        class Global(SoirV8.JSClass):
            def __init__(self):
                self.o = Obj()

        with SoirV8.JSContext(Global()) as ctxt:
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
        class Global(SoirV8.JSClass):
            def __init__(self):
                self.s = self

        with SoirV8.JSContext(Global()) as ctxt:
            self.assertRaises(ReferenceError, ctxt.eval, 'x')

            self.assertTrue(ctxt.eval("typeof(x) === 'undefined'"))

            self.assertTrue(ctxt.eval("typeof(String) === 'function'"))

            self.assertTrue(ctxt.eval("typeof(s.String) === 'undefined'"))

            self.assertTrue(ctxt.eval("typeof(s.z) === 'undefined'"))

    def testRaiseExceptionInGetter(self):
        class Document(SoirV8.JSClass):
            def __getattr__(self, name):
                if name == 'y':
                    raise TypeError()

                return SoirV8.JSClass.__getattr__(self, name)

        class Global(SoirV8.JSClass):
            def __init__(self):
                self.document = Document()

        with SoirV8.JSContext(Global()) as ctxt:
            self.assertEqual(None, ctxt.eval('document.x'))
            self.assertRaises(TypeError, ctxt.eval, 'document.y')

    def testUndefined(self):
        class Global(SoirV8.JSClass):
            def returnNull(self):
                return SoirV8.JSNull()

            def returnUndefined(self):
                return SoirV8.JSUndefined()

            def returnNone(self):
                return None

        with SoirV8.JSContext(Global()) as ctxt:
            self.assertFalse(bool(SoirV8.JSNull()))
            self.assertFalse(bool(SoirV8.JSUndefined()))

            self.assertEqual("null", str(SoirV8.JSNull()))
            self.assertEqual("undefined", str(SoirV8.JSUndefined()))

            self.assertTrue(ctxt.eval('null == returnNull()'))
            self.assertTrue(ctxt.eval('undefined == returnUndefined()'))
            self.assertTrue(ctxt.eval('null == returnNone()'))


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
