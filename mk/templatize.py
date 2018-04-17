#!/usr/bin/env python3
import sys
import os
import re
import time  # used by templates


def templatize(template, output, loc):
  """Process a template file

  '#pragma avarix_tpl CODE' and '$$avarix:CODE$$' sequences are replaced.
  CODE is Python code evaluated with eval() whose result (converted to a string)
  is used as substitute.

  Parameters:
    template -- template file (name or file object)
    output -- output file (name or file object)
    loc -- local values accessible in the template code

  """

  if isinstance(template, str):
    template = open(template, 'r', newline='')
  if isinstance(output, str):
    dout = os.path.dirname(output)
    if not os.path.isdir(dout):
      os.makedirs(dout)
    output = open(output, 'w', newline='')
  def tpl_replace(m):
    code = m.group(1).replace('\\\n', '').strip()
    return str(eval(code, None, loc))
  txt = template.read()
  txt = re.sub(r'\$\$avarix:(.*?)\$\$', tpl_replace, txt)
  pragma_re = re.compile(r'^#pragma\s+avarix_tpl\s+((?:.*\\$)*.*)$\n', re.M)
  txt = pragma_re.sub(tpl_replace, txt)
  output.write(txt)


def main():
  import argparse
  module_name = 'avarix_templatizer'

  parser = argparse.ArgumentParser(
      epilog="""
      Locals are find into a 'template_locals' variable.
      Module or file is loaded with __name__ == %r.
      """ % (module_name,))
  parser.add_argument('-o', '--output', required=True,
      help="output directory or file")
  parser.add_argument('-i', '--inputs', nargs='+',
      help="template files to process")
  parser.add_argument('-q', '--quiet', action='store_true',
      help="suppress normal output")
  parser.add_argument('locals',
    help="module or file defining locals")
  parser.add_argument('args', nargs='*',
    help="sys.argv for the locals' module or file")

  args = parser.parse_args()

  import imp
  sys.argv = [args.locals] + args.args
  if '/' in args.locals or os.path.exists(args.locals):
    mod = imp.load_source(module_name, args.locals)
  else:
    mod = imp.load_module(module_name, *imp.find_module(args.locals))
  loc = mod.template_locals

  if len(args.inputs) == 1:
    tpl = args.inputs[0]
    if args.output.endswith('/') or os.path.isdir(args.output):
      output = os.path.join(args.output, os.path.basename(tpl))
    else:
      output = args.output
    if not args.quiet:
      print("Generate %s -> %s" % (tpl, output))
    templatize(tpl, output, loc)
  else:
    for tpl in args.inputs:
      output = os.path.join(args.output, os.path.basename(tpl))
      if not args.quiet:
        print("Generate %s -> %s" % (tpl, output))
      templatize(tpl, output, loc)


if __name__ == '__main__':
  sys.exit(main())

