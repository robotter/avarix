#!/usr/bin/env python2.7

import re
import sys
import struct
import time
from serial import Serial
from contextlib import contextmanager
from getpass import getuser


def load_hex(f):
  """Parse an HEX file to a list of data chunks
  f is a file object or a filename.

  Return a list of (address, data) pairs.
  """
  if isinstance(f, basestring):
    f = open(f, 'rb')

  ret = []
  addr, data = 0, ''  # current chunk
  offset = 0  # set by record type 02
  for s in f:
    s = s.strip()
    # only do basic checks needed for the algo
    if re.match(':[\dA-Fa-f]{10,42}$', s) is None:
      raise ValueError("invalid HEX line: %s" % s)
    fields = {
        'bytecount':  int(s[1:3],16),
        'address':    int(s[3:7],16),
        'recordtype': int(s[7:9],16),
        'data':       s[9:-2],
        'checksum':   int(s[-2:],16),
        }
    if len(fields['data']) != 2*fields['bytecount']:
      raise ValueError("invalid byte count")

    rt = fields['recordtype']
    if rt == 0:
      newaddr = fields['address'] + offset
      if newaddr < addr+len(data):
        raise ValueError("backward address jump")
      if newaddr > addr+len(data):
        # gap: store previous chunk
        if data:
          ret.append((addr, data))
        addr, data = newaddr, ''
      d = fields['data']
      data += d.decode('hex')
    elif rt == 1:
      break # EOF
    elif rt == 2:
      if fields['bytecount'] != 2:
        raise ValueError("invalid extended segment address record")
      offset = int(fields['data'],16) << 4
    else:
      raise ValueError("invalid HEX record type: %s" % rt)
  # last chunk
  if data:
    ret.append((addr, data))
  return ret


def split_hex_chunks(chunks, size, fill):
  """Split and pad a list of data chunks
  chunks and return value are lists of (address, data) pairs.
  """
  if not len(chunks):
    return []
  ret = []
  paddr, pdata = 0, ''

  for addr, data in chunks:
    if addr < paddr+size:
      # pad previous page
      pdata = pdata.ljust(addr-paddr, fill)
      # append new data to previous page
      n = size - len(pdata)
      pdata += data[:n]
      if len(pdata) < size:
        continue  # page still incomplete
      # start new page
      ret.append((paddr, pdata))
      paddr, pdata, addr, data = addr+n, '', addr+n, data[n:]
    else:
      # pad and push previous page
      if pdata:
        pdata = pdata.ljust(size, fill)
        ret.append((paddr, pdata))
      # start new page, align new data
      n = addr % size
      paddr, pdata = addr-n, fill*n+data

    # push full pages
    while len(data) > size:
      ret.append((addr, data[:size]))
      addr, data = addr+size, data[size:]
    paddr, pdata = addr, data
  # last page
  if pdata:
    pdata = pdata.ljust(size, fill)
    ret.append((paddr, pdata))
  return ret


class TimeoutError(RuntimeError):
  pass


class UserSig:
  """
  User signature data
  """

  sig_fmt = '<B4sL32s'
  sig_fields = ('version', 'device_name', 'prog_date', 'prog_user')

  def __init__(self, **kw):
    self.version = kw['version']
    if self.version is None:
      self.device_name = None
      self.prog_date = None
      self.prog_user = None
    else:
      self.device_name = kw['device_name']
      if len(self.device_name) > 4:
        raise ValueError("invalid device name: %r" % self.device_name)
      self.prog_date = int(kw['prog_date'])
      self.prog_user = kw['prog_user']
      if len(self.prog_user) > 32:
        raise ValueError("user name too long: %r" % self.prog_user)

  @classmethod
  def default(cls, version=1, device_name=None, prog_date=None, prog_user=None):
    """Create a new default signature"""
    return cls(
        version = 1,
        device_name = "----" if device_name is None else device_name,
        prog_date = int(time.time()) if prog_date is None else prog_date,
        prog_user = getuser() if prog_user is None else prog_user,
        )

  def pack(self):
    """Return a binary representation of the signature"""
    return struct.pack(self.sig_fmt,
        self.version, self.device_name, self.prog_date, self.prog_user)

  @classmethod
  def unpack(cls, data):
    """Create a signature instance from binary data"""
    version = ord(data[0])
    fields = {}
    if version in (0, 0xff):
      fields['version'] = None
    else:
      values = struct.unpack(cls.sig_fmt, data[:struct.calcsize(cls.sig_fmt)])
      fields.update(zip(cls.sig_fields, values))
    return cls(**fields)



class BaseClient(object):
  """
  Low-level Client for the Rob'Otter Bootloader
  """

  STATUS_NONE             = 0x00
  STATUS_SUCCESS          = 0x01
  STATUS_BOOTLOADER_MSG   = 0x0a
  STATUS_FAILURE          = 0xff
  STATUS_UNKNOWN_COMMAND  = 0x81
  STATUS_BAD_VALUE        = 0x82
  STATUS_CRC_MISMATCH     = 0x90


  def __init__(self, conn):
    """Initialize the client

    conn is an UART connection object with the following requirements:
     - should not be non-blocking (it may block only for a given period)
     - write() and read(1) must be implemented

    Note: methods assume that their are no pending data in the input queue
    before sending a command. Otherwise, read responses will not match.

    """
    self.conn = conn
    self.__tend = None


  # Data transmission

  def send_raw(self, buf):
    """Send raw data to the server"""
    self.conn.write(buf)
    self.output_write(buf)

  def recv_raw(self, n):
    """Receive raw data from the server"""
    buf = ''
    if self.__tend and time.time() > self.__tend:
      raise TimeoutError()
    while len(buf) < n:
      c = self.conn.read(1)
      if c != '':
        buf += c
      elif self.__tend and time.time() > self.__tend:
        raise TimeoutError()
    self.output_read(buf)
    return buf

  def send_cmd(self, cmd, fmt='', *args):
    """Send a command, return a Reply object

    cmd is either a byte value or a single character string.
    If args is not empty, fmt and args are parameters for struct.pack (without
    byte order indication).
    Otherwise, fmt is the command string to send.
    """
    if not isinstance(cmd, basestring):
      cmd = chr(cmd)
    else:
      ord(cmd) # check the value
    if len(args):
      data = struct.pack('<'+fmt, *args)
    else:
      data = fmt
    self.send_raw(cmd + data)
    return self.recv_reply()

  def recv_reply(self):
    """Read a reply

    Leading null bytes are skipped.
    See Reply for the meaning of fmt.
    Return a Reply object.
    """
    while True:
      size = ord(self.recv_raw(1))
      if size != 0:
        break
    return Reply(self.recv_raw(size))


  # Commands

  def cmd_mirror(self, v):
    """Ping/pong command

    Return True if the same byte was read in the reply, False otherwise.
    """
    r = self.send_cmd('m', 'B', v)
    if not r:
      return False
    return r.unpack('B')[0] == v

  def cmd_infos(self):
    """Retrieve server infos

    Return a the tuple (pagesize,) or False on error.
    """
    r = self.send_cmd('i')
    if not r:
      return False
    pagesize, = r.unpack('H')
    return pagesize,

  def cmd_fuse_read(self):
    """Read fuses

    Return a tuple of fuse bytes or False on error.
    """
    r = self.send_cmd('f')
    if not r:
      return False
    return r.unpack('%dB' % len(r.data))

  def cmd_prog_page(self, addr, buf, crc):
    """Program a page

    Return True on success, False on error and None on CRC mismatch.
    """
    r = self.send_cmd('p', 'IH%ds' % len(buf), addr, crc, buf)
    if r.status == self.STATUS_CRC_MISMATCH:
      return None
    if not r:
      return False
    return True

  def cmd_read_user_sig(self):
    """Read user signature

    Return user signature as a UserSig instance or False on error.
    """
    r = self.send_cmd('s')
    if not r:
      return False
    return UserSig.unpack(r.data)

  def cmd_prog_user_sig(self, sig):
    """Program the user signature

    sig can be a data buffer or a UserSig instance.
    Return True on success, False on error and None on CRC mismatch.
    """
    if isinstance(sig, UserSig):
      sig = sig.pack()
    r = self.send_cmd('S', 'HB%ds' % len(sig), self.compute_crc(sig), len(sig), sig)
    if r.status == self.STATUS_CRC_MISMATCH:
      return None
    if not r:
      return False
    return True

  def cmd_mem_crc(self, start, size):
    """Compute the CRC of a given memory range

    Return the computed CRC or False on error.
    """
    r = self.send_cmd('c', 'II', start, size)
    if not r:
      return False
    return r.unpack('H')[0]

  def cmd_execute(self):
    """Run the application

    Return True on success, False otherwise.
    """
    r = self.send_cmd('x')
    if not r:
      return False
    return True


  # Timeout handling

  @contextmanager
  def timeout(self, t):
    """Return a context to use as a timeout wrapper

    Timeout value is provided in seconds.
    After each read that returned nothing, and before each recv_raw() call, if
    the given duration is expired, a TimeoutError is raised.

    Note that actual behavior highly depends on the connection file object.
    When using PySerial, it depends on the Serial's timeout value.
    """
    if self.__tend is not None:
      raise RuntimeError("recursive timeout() call")
    try:
      self.__tend = time.time() + t
      yield
    finally:
      self.__tend = None


  # Output (default: do nothing)

  def output_read(self, data): pass
  def output_write(self, data): pass


  # Utils

  @staticmethod
  def compute_crc(data):
    """Compute a CRC as used by the protocol"""
    crc = 0xffff
    for c in data:
      c = ord(c)
      c ^= (crc&0xff)
      c ^= c << 4
      c &= 0xff
      crc = ((c << 8) | ((crc>>8)&0xff)) ^ ((c>>4)&0xff) ^ (c << 3)
      crc &= 0xffff
    return crc


class Reply(object):
  """
  Parse a server reply into an object

  Attributes:
    status -- reply status, as an int
    fields -- list of parsed fields
    data -- unparsed field data
    raw -- raw copy of the reply

  The response evaluates to True if it has a non-error status.
  """

  def __init__(self, s):
    self.raw = s
    self.status = ord(s[0])
    self.data = s[1:]
    self.fields = []

  def unpack(self, fmt):
    """Unpack data using a format string

    fmt is a string as used by struct.unpack but without byte order
    indication.
    Unpacked fields are appended to the fields attribute and returned.
    """
    fmt = '<'+fmt  # little-endian
    n = struct.calcsize(fmt)
    l = struct.unpack(fmt, self.data[:n])
    self.fields += l
    self.data = self.data[n:]
    return l

  def unpack_sz(self):
    """Unpack a NUL-terminated string

    The unpacked string is appended to the fields attribute and returned.
    """
    try:
      i = self.data.index('\0')
    except ValueError:
      raise ValueError("NUL byte not found")
    s = self.data[:i]
    self.fields.append(s)
    self.data = self.data[i+1:]
    return s

  def __nonzero__(self):
    return 1 if self.status < 0x80 else 0
  __bool__ = __nonzero__



class ClientError(StandardError):
  pass

class Client(BaseClient):
  """Bootloader client with high level orders

  If the given connection object defines an eof() method which returns True if
  there is nothing to read (a call to read(1) will block) and False otherwise
  or an inWaiting() method (as defined by pyserial objects), it will be used
  for a better (and safer) behavior.

  Attributes:
    pagesize -- server's page size

  """

  CRC_RETRY = 5  # maximum number of retry on CRC mismatch
  UNUSED_BYTE = '\xFF'  # value of programmed unused bytes

  def __init__(self, conn, **kw):
    BaseClient.__init__(self, conn)

    if hasattr(conn, 'eof') and callable(conn.eof):
      self._eof = conn.eof
    elif hasattr(conn, 'inWaiting') and callable(conn.inWaiting):
      self._eof = lambda: conn.inWaiting() == 0
    else:
      self._eof = None

    # send init string, if any
    if kw.get('init_send'):
      self.send_raw(kw.get('init_send'))

    self.clear_infos()

  __sync_gen = None

  @classmethod
  def _sync_byte(cls):
    """Return a synchronization byte to use"""
    if cls.__sync_gen is None:
      def gen():
        vals = range(1, 255)
        import random
        random.shuffle(vals)
        while True:
          for i in vals:
            yield i
      cls.__sync_gen = gen()
    return next(cls.__sync_gen)

  def synchronize(self):
    """Synchronize with the server to be ready to send a command"""
    #XXX current implementation is lazy
    # We wait for the expected reply to a mirror command.
    # On unexpected data, a new mirror command is sent.
    while True:
      # flush first
      if self._eof is not None:
        while not self._eof():
          self.recv_raw(1)
      # send a mirror command
      # don't use cmd_mirror() to control reply reading
      sync = chr(self._sync_byte())
      self.send_raw('m'+sync)
      if (self.recv_raw(1) == '\x02' and  # size
          self.recv_raw(1) == '\x01' and  # status
          self.recv_raw(1) == sync):
        break
      # wait a bit to receive all pending data before flushing it
      time.sleep(0.1)


  def program(self, fhex, fhex2=None, user_sig=None):
    """Send a program to the device

    Parameters:
      fhex -- HEX data to program
      fhex2 -- previous HEX data, to program changes
      user_sig -- update the user signature

    See parse_hex() for accepted fhex/fhex2 values.
    See program_pages() for return values.

    """
    pages = self.parse_hex(fhex)
    if fhex2 is None:
      return self.program_pages(pages, user_sig=user_sig)
    pages2 = self.parse_hex(fhex2)
    return self.program_pages(self.diff_pages(pages, pages2), user_sig=user_sig)


  def program_pages(self, pages, user_sig=None):
    """Send a program to the device

    Parameters:
      pages -- a list of pages to program
      user_sig -- update the user signature

    pages is a list of (address, data) pairs. address has to be aligned to
    pagesize, data is padded if needed but size must not exceed page size.
    If pages is a single string, it will be cut in pages starting at address 0.

    Return True on success, None if nothing has been programmed.

    """

    page_count = len(pages)
    if page_count == 0:
      return None

    self.synchronize()

    if user_sig is not None:
      if isinstance(user_sig, UserSig):
        user_sig = user_sig.pack()
      n = self.CRC_RETRY
      while True:
        r = self.cmd_prog_user_sig(user_sig)
        if r is True:
          break # ok
        elif r is False:
          raise ClientError("user signature prog failed")
        elif r is None:
          if n == 0:
            raise ClientError("user signature prog failed %s: too many attempts")
          n = n-1
          continue # retry
        raise ValueError("unexpected cmd_prog_user_sig() return value: %r" % r)

    for i, (addr, data) in enumerate(pages):
      self.output_program_progress(i+1, page_count)

      err_info = "at page %d/%d (0x%0x)" % (i+1, page_count, addr)
      data = data.ljust(self.pagesize, self.UNUSED_BYTE)
      assert addr % self.pagesize == 0, "page address not aligned %s" % err_info
      assert len(data) == self.pagesize, "invalid page size %s" % err_info

      crc = self.compute_crc(data)
      n = self.CRC_RETRY
      while True:
        r = self.cmd_prog_page(addr, data, crc)
        if r is True:
          break # ok
        elif r is False:
          raise ClientError("prog failed %s" % err_info)
        elif r is None:
          if n == 0:
            raise ClientError("prog failed %s: too many attempts" % err_info)
          n = n-1
          continue # retry
        raise ValueError("unexpected cmd_prog_page() return value: %r" % r)
    self.output_program_end()
    return True


  def check(self, fhex, fill=True):
    """Check the program on the device

    Parameters:
      fhex -- HEX data to check (HEX file/filename or list of (addr, data))
      gap -- if True, gaps in checked data are filled with UNUSED_BYTE,
             otherwise several check commands are sent

    Return True if the CRC matches, False otherwise
    """

    # don't parse to pages: this may add unwanted padding
    if isinstance(fhex, (tuple, list)):
      chunks = fhex
    else:
      chunks = load_hex(fhex)
    if len(chunks) == 0:
      raise ValueError("empty HEX data")
    # merge chunks, fill gaps if needed
    chunks2 = []
    curaddr, curdata = 0, ''
    for addr,data in chunks:
      if addr < curaddr+len(curdata):
        raise ValueError("backward address jump")
      elif addr == curaddr:
        # merge
        curdata += data
      elif not curdata:
        # new chunk
        curaddr, curdata = addr, data
      elif fill:
        # fill gaps
        curdata += (addr-curaddr-len(curdata)) * self.UNUSED_BYTE + data
      else:
        # new chunk
        chunks2.append((curaddr, curdata))
        curaddr, curdata = addr, data
    # last chunk
    if curdata:
      chunks2.append((curaddr, curdata))

    self.synchronize()
    for addr,data in chunks2:
      r = self.cmd_mem_crc(addr, len(data))
      if r is False:
        raise ClientError("crc check failed")
      if r != self.compute_crc(data):
        return False
    return True


  def update_infos(self):
    """Get device infos, update associated attributes"""
    self.synchronize()
    r = self.cmd_infos()
    if not r:
      raise ClientError("cannot retrieve device info")
    self.pagesize, = r
    return r

  def clear_infos(self):
    """Clear cached device infos"""
    self.pagesize = None


  def read_fuses(self):
    """Read fuses and return a tuple of bytes"""
    self.synchronize()
    r = self.cmd_fuse_read()
    if r is False:
      raise ClientError("failed to read fuses")
    return r

  def read_user_sig(self):
    """Read user signature, return a UserSig instance"""
    self.synchronize()
    r = self.cmd_read_user_sig()
    if r is False:
      raise ClientError("failed to read user signature")
    return r


  def boot(self):
    """Boot the device"""
    self.synchronize()
    r = self.cmd_execute()
    if r is False:
      raise ClientError("boot failed")
    return


  def parse_hex(self, fhex):
    """Parse an HEX file to a list of pages.
    Raise an exception if data is empty.
    If f is already a list of pages, return it unchanged.
    """
    if isinstance(fhex, (tuple, list)):
      pages = fhex
    else:
      data = load_hex(fhex)
      if self.pagesize is None:
        self.update_infos()
      pages = split_hex_chunks(data, self.pagesize, self.UNUSED_BYTE)
    if not len(pages):
      raise ValueError("empty HEX data")
    return pages

  @classmethod
  def diff_pages(cls, p1, p2):
    """Return the pages from p1 not in p2"""
    return sorted(set(p1) - set(p2))

  # Output
  def output_read(self, data):
    pass
  def output_write(self, data):
    pass
  def output_program_progress(self, ncur, nmax):
    pass
  def output_program_end(self):
    pass



def main():
  from argparse import ArgumentParser

  # default port, platform-dependant
  import os
  if os.name == 'nt':
    default_port = 'COM1'
  else:
    default_port = '/dev/ttyUSB0'
  default_baudrate = 38400

  parser = ArgumentParser(
      description="Bootloader Client",
      usage="%(prog)s [OPTIONS] [file.hex] [previous.hex]",
      )
  parser.add_argument('-P', '--port', dest='port', default=default_port,
      help="device port to used (default: %s)" % default_port)
  parser.add_argument('-s', '--baudrate', type=int, default=default_baudrate,
      help="baudrate (default: %d)" % default_baudrate)
  parser.add_argument('-p', '--program', action='store_true',
      help="program the device")
  parser.add_argument('-c', '--check', action='store_true',
      help="check current loaded program")
  parser.add_argument('-b', '--boot', action='store_true',
      help="boot (after programming and CRC check)")
  parser.add_argument('-f', '--read-fuses', action='store_true',
      help="print value of fuse bytes")
  parser.add_argument('--read-user-sig', action='store_true',
      help="print value of user signature")
  parser.add_argument('--device-name',
      help="device name, use in user signature, check before programming")
  parser.add_argument('--keep-user-sig', action='store_true',
      help="don't update user signature")
  parser.add_argument('--force-device-name', action='store_true',
      help="don't check device name")
  parser.add_argument('--init-send', dest='init_send',
      help="string to send at connection (eg. to reset the device)")
  parser.add_argument('-v', '--verbose', action='store_true',
      help="print transferred data on stderr")
  parser.add_argument('hex', nargs='?',
      help="HEX file to program or check")
  parser.add_argument('previous', nargs='?',
      help="previous HEX file, to only program changes")
  args = parser.parse_args()

  # check positional arguments against other options
  if args.hex is None:
    if args.program or args.check:
      parser.error("missing argument: HEX file")
  if args.previous is not None:
    if not args.program and args.previous:
      parser.argument("extra argument: previous HEX file")

  # connect to serial line and setup stdin/out
  conn = Serial(args.port, args.baudrate, timeout=0.5)

  class CliClient(Client):
    def __init__(self, conn, **kw):
      self.verbose = kw.get('verbose', False)
      Client.__init__(self, conn, **kw)

    def output_read(self, data):
      if self.verbose:
        sys.stderr.write(" << %r\n" % data)
    def output_write(self, data):
      if self.verbose:
        sys.stderr.write(" >> %r\n" % data)

    def output_program_progress(self, ncur, nmax):
      sys.stdout.write("\rprogramming: page %3d / %3d  -- %2.2f%%"
                            % (ncur, nmax, (100.0*ncur)/nmax))
      sys.stdout.flush()
    def output_program_end(self):
      sys.stdout.write("\n")


  client = CliClient(conn, verbose=args.verbose, init_send=args.init_send)
  print "bootloader waiting..."
  client.synchronize()
  client.update_infos()

  if not args.force_device_name and args.device_name is not None:
    sig = client.read_user_sig()
    if sig.device_name is not None and sig.device_name != args.device_name:
      print "device name mismatch: got %r, expected %r" % (sig.device_name, args.device_name)
      return
  if args.read_fuses:
    fuses = client.read_fuses()
    print ("fuses:"+' %02x'*len(fuses)) % fuses
  if args.program:
    print "programming..."
    if args.keep_user_sig:
      sig = None
    else:
      sig = UserSig.default(device_name=args.device_name)
    ret = client.program(args.hex, args.previous, user_sig=sig)
    if ret is None:
      print "nothing to program"
  if args.check:
    print "CRC check..."
    if client.check(args.hex, False):
      print "CRC OK"
    else:
      print "CRC mismatch"
      args.boot = False
  if args.read_user_sig:
    sig = client.read_user_sig()
    if sig.version is None:
      print "user signature: no data"
    else:
      print "user signature (version %d)" % sig.version
      print "  device name: %s" % sig.device_name
      print "  prog date: %s" % time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(sig.prog_date))
      print "  prog user: %s" % sig.prog_user
  if args.boot:
    print "boot"
    client.boot()


if __name__ == '__main__':
  main()

