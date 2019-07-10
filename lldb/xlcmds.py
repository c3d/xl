#!/usr/bin/python

import lldb
import fblldbbase as fb

def lldbcommands():
  return [
    PrintDebugInformation(),
    PrintUnifications(),
    PrintTypes(),
    PrintMachineTypes(),
    PrintRewriteCalls(),
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


class PrintUnifications(fb.FBCommand):
  def name(self):
    return 'xlu'

  def description(self):
    return "Show XL type unifications"

  def run(self, arguments, options):
    lldb.debugger.HandleCommand('p XL::Types::DumpUnifications()')


class PrintTypes(fb.FBCommand):
  def name(self):
    return 'xlt'

  def description(self):
    return "Show XL types"

  def run(self, arguments, options):
    lldb.debugger.HandleCommand('p XL::Types::DumpTypes()')


class PrintMachineTypes(fb.FBCommand):
  def name(self):
    return 'xlm'

  def description(self):
    return "Show XL machine types"

  def run(self, arguments, options):
    lldb.debugger.HandleCommand('p XL::Types::DumpMachineTypes()')


class PrintRewriteCalls(fb.FBCommand):
  def name(self):
    return 'xlr'

  def description(self):
    return "Show XL rewrite calls"

  def run(self, arguments, options):
    lldb.debugger.HandleCommand('p XL::Types::DumpRewriteCalls()')


class RecorderDump(fb.FBCommand):
  def name(self):
    return 'rdump'

  def description(self):
    return "Dump the flight recorder"

  def run(self, arguments, options):
    lldb.debugger.HandleCommand('p recorder_dump()')
