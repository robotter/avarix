from templatize import templatize
import rome


class CodeGenerator:
  """
  Generate code for AVR

  Attributes:
    messages -- list ROME messages, sorted by message ID

  """

  def __init__(self):
    self.messages = sorted(rome.messages.values(), key=lambda m: m.mid)

  @classmethod
  def c_typedecl(cls, typ, name):
    """Return C type declaration of a ROME type"""
    if issubclass(typ, rome.types.EnumType):
      return 'rome_enum_%s_t %s' % (typ.name, name)
    elif issubclass(typ, rome.types.rome_int):
      u = 'u' if not typ.signed else ''
      n = typ.packsize * 8
      return '%sint%d_t %s' % (u, n, name)
    elif issubclass(typ, rome.types.rome_float):
      return 'float %s' % name
    elif issubclass(typ, rome.types.ArrayType):
      return '%s[%d]' % (cls.c_typedecl(typ.base, name), typ.array_size)
    elif issubclass(typ, rome.types.VarArrayType):
      return '%s[0]' % cls.c_typedecl(typ.base, name)
    elif issubclass(typ, rome.types.rome_string):
      return 'char %s[0]' % name
    else:
      raise TypeError("unsupported type: %s" % typ)

  @classmethod
  def mid_enum_name(cls, msg):
    return 'ROME_MID_%s' % msg.name.upper()

  def mid_enum_fields(self):
    return ''.join(
        '  %s = 0x%02X,\n' % (self.mid_enum_name(msg), msg.mid)
        for msg in self.messages)

  @classmethod
  def type_enum_name(cls, msg, key):
    return 'ROME_ENUM_%s_%s' % (msg.name.upper(), key.upper())

  def enum_types(self):
    # gather enum types from all messages
    enums = set( t
        for msg in self.messages
        for v,t in msg.ptypes
        if issubclass(t, rome.types.EnumType) )
    ret = ''
    for enum in enums:
      values = ''.join( '  %s = %d,\n' % (self.type_enum_name(enum, k), v)
          for k,v in sorted(enum.values.items()) )
      ret += "typedef enum {\n%s} rome_enum_%s_t;\n\n" % (values, enum.name)
    return ret


  def msgdata_union_fields(self):
    ret = ''
    for msg in self.messages:
      fields = [ self.c_typedecl(t, v) + ';' for v,t in msg.ptypes ]
      if isinstance(msg, rome.frame.Order):
        fields.insert(0, 'uint8_t _ack;')
      ret += '\n    struct {\n%s    } %s;\n' % (
          ''.join( '      %s\n'%s for s in fields ),
          msg.name,
          )
    return ret

  def max_param_size(self):
    # always use the maximum value
    # the log message will always "force" this
    return 255

  @classmethod
  def msg_macro_helper(cls, msg):
    pnames = []
    set_params = ''
    for v,t in msg.ptypes:
      if issubclass(t, rome.types.rome_string):
        pnames.append('_a_%s' % v)
        set_params += '  strcpy((_f)->%(name)s.%(v)s, _a_%(v)s); \\\n' % {
            'name': msg.name, 'v': v }
      elif issubclass(t, rome.types.ArrayType):
        pnames.append('_a_%s' % v)
        set_params += '  memcpy((_f)->%(name)s.%(v)s, _a_%(v)s, %(asize)d * %(psize)d); \\\n' % {
            'name': msg.name, 'v': v, 'asize': t.array_size, 'psize': t.base.packsize }
      elif issubclass(t, rome.types.VarArrayType):
        pnames.extend(('_a_%s' % v, '_n_%s' % v))
        set_params += '  memcpy((_f)->%(name)s.%(v)s, _a_%(v)s, _n_%(v)s * %(psize)d); \\\n' % {
            'name': msg.name, 'v': v, 'psize': t.base.packsize }
      else:
        pnames.append('_a_%s' % v)
        set_params += '  (_f)->%s.%s = (_a_%s); \\\n' % (msg.name, v, v)
    if isinstance(msg, rome.frame.Order):
      param_ack = ', _a_ack'
      paren_ack = ', (_a_ack)'
      set_ack = '  (_f)->%s._ack = (_a_ack); \\\n' % msg.name
    else:
      param_ack = ''
      paren_ack = ''
      set_ack = ''

    tpl = (
        '#define ROME_SET_%(NAME)s(_f%(param_ack)s%(pnames)s) do { \\\n'
        '  (_f)->plsize = %(plsize)s; \\\n'
        '  (_f)->mid = %(MID)s; \\\n'
        '%(set_ack)s%(set_params)s'
        '} while(0)\n'
        '\n'
        '#define ROME_SEND_%(NAME)s(_i%(param_ack)s%(pnames)s) do { \\\n'
        '  uint8_t _buf[2+%(plsize)s]; \\\n'
        '  rome_frame_t *_frame = (rome_frame_t*)_buf; \\\n'
        '  ROME_SET_%(NAME)s(_frame%(paren_ack)s%(paren_pnames)s); \\\n'
        '  rome_send((_i), _frame); \\\n'
        '} while(0)\n'
        '\n'
        )

    if isinstance(msg, rome.frame.Order):
      tpl += (
          '#ifdef ROME_ACK_MIN\n'
          '#define ROME_SENDWAIT_%(NAME)s(_i%(pnames)s) do { \\\n'
          '  uint8_t _buf[2+%(plsize)s]; \\\n'
          '  rome_frame_t *_frame = (rome_frame_t*)_buf; \\\n'
          '  ROME_SET_%(NAME)s(_frame, 0%(paren_pnames)s); \\\n'
          '  rome_sendwait((_i), _frame); \\\n'
          '} while(0)\n'
          '#endif\n'
          '\n'
          )

    return tpl % {
            'NAME': msg.name.upper(),
            'pnames': ''.join(', '+s for s in pnames),
            'paren_pnames': ''.join(', (%s)' % s for s in pnames),
            'plsize': msg.plsize,
            'MID': cls.mid_enum_name(msg),
            'set_params': set_params,
            'param_ack': param_ack,
            'paren_ack': paren_ack,
            'set_ack': set_ack,
            }

  def macro_helpers(self):
    ret = ''
    for msg in self.messages:
      ret += self.msg_macro_helper(msg)
    return ret

  @classmethod
  def msg_macro_disabler(cls, msg):
    return (
        '#ifdef ROME_DISABLE_%s\n'
        '# define %s 0\n'
        '#endif\n'
        ) % (msg.name.upper(), cls.mid_enum_name(msg))

  def macro_disablers(self):
    ret = ''
    for msg in self.messages:
      if not msg.varsize:
        ret += self.msg_macro_disabler(msg)
    return ret


if __name__ == 'avarix_templatizer':
  import imp
  import sys
  import os
  module_name = 'avarix_templatizer.rome_transactions'
  if len(sys.argv) >= 2:
    messages = sys.argv[1]
    if '/' in messages or os.path.exists(messages):
      mod = imp.load_source(module_name, messages)
    else:
      mod = imp.load_module(module_name, *imp.find_module(messages))
  if len(rome.messages) == 0:
    raise RuntimeError("no defined messages, define ROME_MESSAGES in Makefile")
  template_locals = {'self': CodeGenerator()}


