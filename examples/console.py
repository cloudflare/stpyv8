#!/usr/bin/env python

# console.py - run javascipt using a custom console

from logging import getLogger, basicConfig, INFO
import STPyV8 as V8

basicConfig(format='%(asctime)-15s %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S')
log = getLogger('myapp')
log.setLevel(INFO)

class Console(object):
  def log(self, message):
    log.info(message)

  def error(self, message):
    log.error(message)

class Global(V8.JSClass):
  custom_console = Console()

def load(js_file):
  with open(js_file, 'r') as f:
    return f.read()

def run(js_file):
  with V8.JSContext(Global()) as ctxt:
    ctxt.eval('this.console = custom_console;')
    ctxt.eval(load(js_file))

run('simple.js')

