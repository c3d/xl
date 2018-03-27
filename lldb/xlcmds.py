#!/usr/bin/python

import lldb
import fblldbbase as fb

def lldbcommands():
  return [
    PrintDebugInformation(),
    RecorderDump()
  ]


class PrintDebugInformation(fb.FBCommand):
  def name(self):
    return 'xl'

  def description(self):
    return "Pretty-print an XL compiler entity"

  def args(self):
    return [
      fb.FBCommandArgument(arg='object',
                           type='XL compiler type',
                           help='Value to print.')
    ]

  def run(self, arguments, options):
    lldb.debugger.HandleCommand('p xldebug(%s)' % arguments[0])


class RecorderDump(fb.FBCommand):
  def name(self):
    return 'rdump'

  def description(self):
    return "Dump the flight recorder"

  def run(self, arguments, options):
    lldb.debugger.HandleCommand('p recorder_dump()')
