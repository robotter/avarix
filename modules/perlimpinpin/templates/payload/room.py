from templatize import templatize
import perlimpinpin.payload.room as room
from perlimpinpin.payload.room.transaction import Command, Event


class CodeGenerator:
  """
  Generate code for AVR

  Attributes:
    transactions -- transactions for which files are generated
    messages -- all transaction messages

  """

  def __init__(self, transactions):
    self.transactions = transactions
    self.messages = []
    for t in self.transactions:
      self.messages += t.messages()

  @classmethod
  def c_typename(cls, typ):
    """Return C typename of a ROOM type"""
    if issubclass(typ, room.types.room_int):
      u = 'u' if not typ.signed else ''
      n = typ.packsize * 8
      return '%sint%d_t' % (u, n)
    elif issubclass(typ, room.types.room_float):
      return 'float'
    else:
      raise TypeError("unsupported type: %s" % typ)

  @classmethod
  def mid_enum_name(cls, msg):
    return 'ROOM_MID_%s' % msg.mname.upper()

  def mid_enum_fields(self):
    return ''.join(
        '  %s = 0x%02X,\n' % (self.mid_enum_name(msg), msg.mid)
        for msg in self.messages
        )

  def msgdata_union_fields(self):
    ret = ''
    for msg in self.messages:
      fields = ( '%s %s;' % (self.c_typename(t), v) for v,t in msg.ptypes )
      ret += '\n    struct {\n%s    } %s;\n' % (
          ''.join( '      %s\n'%s for s in fields ),
          msg.mname,
          )
    return ret

  def max_param_size(self):
    return max(sum(t.packsize for v,t in msg.ptypes) for msg in self.messages)

  def check_payload_size_switch(self, var):
    ret = ''
    # group messages by size
    size2msgs = {}
    for msg in self.messages:
      size = sum(t.packsize for v,t in msg.ptypes)
      if size not in size2msgs:
        size2msgs[size] = []
      size2msgs[size].append(msg)
    # build switch entries
    for size in sorted(size2msgs):
      for msg in size2msgs[size]:
        ret += '    case %s:\n' % self.mid_enum_name(msg)
      ret += (
          '      %s = %u;\n'
          '      break;\n'
          ) % (var, size)
    return ret


  @classmethod
  def msg_macro_pnames(cls, msg):
    return [ '_a_%s'%v for v,t in msg.ptypes ]

  @classmethod
  def msg_macro_helper(cls, msg):
    pnames = ['_i','_d'] + cls.msg_macro_pnames(msg)
    psize = sum(t.packsize for v,t in msg.ptypes)
    return (
        '#define ROOM_MSG_%s(%s) do { \\\n'
        '  const ppp_header_t _header = { \\\n'
        '    .plsize = 2+%u, \\\n'
        '    .src = (_i)->addr, \\\n'
        '    .dst = (_d), \\\n'
        '    .pltype = PPP_TYPE_ROOM, \\\n'
        '  }; \\\n'
        '  room_payload_t _data = { \\\n'
        '    .mid = %s, \\\n'
        '  }; \\\n'
        '%s'
        '  ppp_send_frame((_i), &_header, &_data); \\\n'
        '} while(0);\n\n') % (
            msg.mname.upper(), ', '.join(pnames),
            psize, cls.mid_enum_name(msg),
            ''.join(
              '  (_data).%s.%s = (_a_%s); \\\n' % (msg.mname, v, v)
              for v,t in msg.ptypes
              ),
            )

  @classmethod
  def forward_msg_macro_helper(cls, prefix, name, msg, params, intf, dst):
    pnames = cls.msg_macro_pnames(msg)
    return (
        '#define ROOM_%s_%s(%s) \\\n'
        '    ROOM_MSG_%s(%s)\n\n') % (
            prefix, name.upper(), ', '.join(list(params) + pnames),
            msg.mname.upper(), ', '.join([intf, dst] + pnames),
        )

  @classmethod
  def trans_macro_helpers(cls, trans):
    ret = ''
    if isinstance(trans, Command):
      ret += cls.forward_msg_macro_helper(
          'SEND', trans.name, trans.request,
          ['_i', '_d'], '(_i)', '(_d)')
      ret += cls.forward_msg_macro_helper(
          'REPLY', trans.name, trans.response,
          ['_i', '_m'], '(_i)', '(_i)->rstate.header.src')
    elif isinstance(trans, Event):
      ret += cls.forward_msg_macro_helper(
          'SEND', trans.name, trans.message,
          ['_i', '_d'], '(_i)', '(_d)')
    else:
      raise TypeError("unsupported transaction type: %r" % trans)
    return ret

  def send_helpers(self):
    ret = ''
    for trans in self.transactions:
      ret += self.trans_macro_helpers(trans)
    ret += '\n'
    for msg in self.messages:
      ret += self.msg_macro_helper(msg)
    return ret


if __name__ == 'avarix_templatizer':
  import imp
  import sys
  import os
  module_name = 'avarix_templatizer.room_transactions'
  transactions = sys.argv[1]
  if '/' in transactions or os.path.exists(transactions):
    mod = imp.load_source(module_name, transactions)
  else:
    mod = imp.load_module(module_name, *imp.find_module(transactions))
  template_locals = {'self': CodeGenerator(mod.transactions)}

