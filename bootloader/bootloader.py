#!/usr/bin/env python2.7
from __future__ import print_function
import re
import sys
import struct
import time
import threading
from serial import Serial
from contextlib import contextmanager
from getpass import getuser
import rome

try:
  basestring
except NameError:
  basestring = str


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


class BaseClient(rome.Client):
  """
  Base class for bootloader client

  Attributes:
    cur_order -- current order waiting for a reply, or None
    cur_reply -- reply to current order
    cur_reply_cond -- condition to wait for current reply

  """

  msg_bootloader = rome.frame.messages_by_name['bootloader']
  msg_bootloader_r = rome.frame.messages_by_name['bootloader_r']

  # maximum payload size for buffer command
  # use 84 so that a full buffer command will fit into an XBeeAPI frame
  buffer_chunk_size = 84

  def __init__(self, fo):
    rome.Client.__init__(self, fo)
    self.cur_order = None
    self.cur_reply = None
    self.cur_reply_cond = threading.Condition()
    # use default values more suitable to bootloader
    # (programming page takes time)
    self.retry_count = 5
    self.retry_period = 0.5

  def on_frame(self, frame):
    with self.cur_reply_cond:
      self.output_recv_frame(frame)
      if self.cur_order is None:
        return  # no current order, nothing to process
      if getattr(frame.params, 'ack', None) != self.cur_order.ack:
        return  # not a reply to the current order
      if frame.msg is not self.msg_bootloader_r:
        return  # should not happen (sanity check)
      self.cur_reply = frame
      self.cur_reply_cond.notify()

  def on_sent_frame(self, frame):
    self.output_send_frame(frame)


  def send_cmd(self, cmd, data=''):
    """Send a command, wait for reply frame
    Return the reply frame or None on timeout.
    """

    if self.cur_order is not None:
      raise RuntimeError("simultaneous bootloader orders")
    with self.cur_reply_cond:
      try:
        ack = rome.Frame.next_ack()
        self.cur_order = rome.Frame(self.msg_bootloader, cmd, data, _ack=ack)
        for i in range(self.retry_count):
          self.cur_reply = None
          self.send(self.cur_order)
          self.cur_reply_cond.wait(self.retry_period)
          if self.cur_reply:
            return self.cur_reply
        return None
      finally:
        self.cur_order = self.cur_reply = None

  def send_buffer(self, buf):
    """Send a whole buffer using buffer commands
    Return True on success or None on timeout.
    """

    if self.cur_order is not None:
      raise RuntimeError("simultaneous bootloader orders")
    #TODO allow to parallelize several chunks
    with self.cur_reply_cond:
      try:
        chunk_size = self.buffer_chunk_size
        offsets = [ (o, chunk_size) for o in range(0, len(buf), chunk_size) ]
        offsets.append((0, 0))
        for offset, size in offsets:
          data = buf[offset:offset+size]
          data = struct.pack('<HB', offset, len(data)) + data
          ack = rome.Frame.next_ack()
          self.cur_order = rome.Frame(self.msg_bootloader, 'buffer', data, _ack=ack)
          for i in range(self.retry_count):
            self.cur_reply = None
            self.send(self.cur_order)
            self.cur_reply_cond.wait(self.retry_period)
            if self.cur_reply:
              if self.cur_reply.params.status == 'success':
                break  # next chunk
          else:
            return None
        return True
      finally:
        self.cur_order = self.cur_reply = None



  def cmd_info(self):
    """Retrieve server information

    Return the tuple (pagesize,) or False on error.
    """
    r = self.send_cmd('info')
    if r is None or r.params.status != 'success':
      return False
    return struct.unpack('<H', r.params.data)

  def cmd_fuse_read(self):
    """Read fuses

    Return a tuple of fuse bytes or False on error.
    """
    r = self.send_cmd('fuse_read')
    if r is None or r.params.status != 'success':
      return False
    return struct.unpack('<%dB' % len(r.params.data), r.params.data)

  def cmd_prog_page(self, addr, buf):
    """Program a page

    Return True on success, False on error.
    """
    data = struct.pack('<I', addr)
    r = self.send_cmd('prog_page', data)
    if r is None:
      return False
    elif r.params.status != 'success':
      return False
    return self.send_buffer(buf)

  def cmd_read_user_sig(self):
    """Read user signature

    Return user signature as a UserSig instance or False on error.
    """
    r = self.send_cmd('read_user_sig')
    if r is None or r.params.status != 'success':
      return False
    return UserSig.unpack(r.params.data)

  def cmd_prog_user_sig(self, sig):
    """Program the user signature

    sig can be a data buffer or a UserSig instance.
    Return True on success, False on error.
    """
    if isinstance(sig, UserSig):
      sig = sig.pack()
    r = self.send_cmd('prog_user_sig', '')
    if r is None:
      return False
    elif r.params.status != 'success':
      return False
    return self.send_buffer(sig)

  def cmd_mem_crc(self, start, size):
    """Compute the CRC of a given memory range

    Return the computed CRC or False on error.
    """
    data = struct.pack('<II', start, size)
    r = self.send_cmd('mem_crc', data)
    if r is None or r.params.status != 'success':
      return False
    return struct.unpack('<H', r.params.data)[0]

  def cmd_boot(self):
    """Run the application

    Return True on success, False otherwise.
    """
    r = self.send_cmd('boot')
    if r is None or r.params.status != 'success':
      return False
    return True

  def cmd_buffer(self, buf, offset, size):
    """Send part of a buffer using the buffer command"""
    data = struct.pack('<HH', offset, size) + buf[offset:offset+size]
    r = self.send_cmd('buffer', data)
    if r is None or r.params.status != 'success':
      return False
    return True


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


class ClientError(StandardError):
  pass

class Client(BaseClient):
  """Bootloader client with high level orders

  Attributes:
    pagesize -- server's page size

  """

  CRC_RETRY = 5  # maximum number of retry on CRC mismatch
  UNUSED_BYTE = '\xFF'  # value of programmed unused bytes

  def __init__(self, conn, **kw):
    BaseClient.__init__(self, conn)
    self.clear_info()


  def synchronize(self):
    """Wait for bootloader to enter"""
    pass #TODO


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

    # program user signature
    if user_sig is not None:
      if isinstance(user_sig, UserSig):
        user_sig = user_sig.pack()
      while True:
        r = self.cmd_prog_user_sig(user_sig)
        if r is True:
          break # ok
        elif r is False:
          raise ClientError("user signature prog failed")
        raise ValueError("unexpected cmd_prog_user_sig() return value: %r" % r)

    for i, (addr, data) in enumerate(pages):
      self.output_program_progress(i+1, page_count)

      err_info = "at page %d/%d (0x%0x)" % (i+1, page_count, addr)
      data = data.ljust(self.pagesize, self.UNUSED_BYTE)
      assert addr % self.pagesize == 0, "page address not aligned %s" % err_info
      assert len(data) == self.pagesize, "invalid page size %s" % err_info

      while True:
        r = self.cmd_prog_page(addr, data)
        if r is True:
          break # ok
        elif r is False:
          raise ClientError("prog failed %s" % err_info)
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

    for addr,data in chunks2:
      r = self.cmd_mem_crc(addr, len(data))
      if r is False:
        raise ClientError("crc check failed")
      if r != self.compute_crc(data):
        return False
    return True


  def update_info(self):
    """Get device info, update associated attributes"""
    r = self.cmd_info()
    if not r:
      raise ClientError("cannot retrieve device info")
    self.pagesize, = r
    return r

  def clear_info(self):
    """Clear cached device info"""
    self.pagesize = None


  def read_fuses(self):
    """Read fuses and return a tuple of bytes"""
    r = self.cmd_fuse_read()
    if r is False:
      raise ClientError("failed to read fuses")
    return r

  def read_user_sig(self):
    """Read user signature, return a UserSig instance"""
    r = self.cmd_read_user_sig()
    if r is False:
      raise ClientError("failed to read user signature")
    return r


  def boot(self):
    """Boot the device"""
    r = self.cmd_boot()
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
        self.update_info()
      pages = split_hex_chunks(data, self.pagesize, self.UNUSED_BYTE)
    if not len(pages):
      raise ValueError("empty HEX data")
    return pages

  @classmethod
  def diff_pages(cls, p1, p2):
    """Return the pages from p1 not in p2"""
    return sorted(set(p1) - set(p2))

  # Output
  def output_recv_frame(self, data):
    pass
  def output_send_frame(self, data):
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
      parser.error("extra argument: previous HEX file")

  # connect to serial line and setup stdin/out
  conn = Serial(args.port, args.baudrate, timeout=0.5)

  class CliClient(Client):
    def __init__(self, conn, **kw):
      self.verbose = kw.get('verbose', False)
      Client.__init__(self, conn, **kw)

    def output_recv_frame(self, frame):
      if self.verbose:
        sys.stderr.write(" << %r\n" % frame)
    def output_send_frame(self, frame):
      if self.verbose:
        sys.stderr.write(" >> %r\n" % frame)

    def output_program_progress(self, ncur, nmax):
      sys.stdout.write("\rprogramming: page %3d / %3d  -- %2.2f%%"
                            % (ncur, nmax, int((100.0*ncur)//nmax)))
      sys.stdout.flush()
    def output_program_end(self):
      sys.stdout.write("\n")


  rome.Frame.set_ack_range(128, 256)
  client = CliClient(conn, verbose=args.verbose)
  client.start()
  print("bootloader waiting...")
  while True:
    try:
      client.update_info()
      break
    except ClientError:
      pass

  if not args.force_device_name and args.device_name is not None:
    sig = client.read_user_sig()
    if sig.device_name is not None and sig.device_name != args.device_name:
      print("device name mismatch: got %r, expected %r" % (sig.device_name, args.device_name))
      return
  if args.read_fuses:
    fuses = client.read_fuses()
    print(("fuses:"+' %02x'*len(fuses)) % fuses)
  if args.program:
    print("programming...")
    if args.keep_user_sig:
      sig = None
    else:
      sig = UserSig.default(device_name=args.device_name)
    ret = client.program(args.hex, args.previous, user_sig=sig)
    if ret is None:
      print("nothing to program")
  if args.check:
    print("CRC check...")
    if client.check(args.hex, False):
      print("CRC OK")
    else:
      print("CRC mismatch")
      args.boot = False
  if args.read_user_sig:
    sig = client.read_user_sig()
    if sig.version is None:
      print("user signature: no data")
    else:
      print("user signature (version %d)" % sig.version)
      print("  device name: %s" % sig.device_name)
      print("  prog date: %s" % time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(sig.prog_date)))
      print("  prog user: %s" % sig.prog_user)
  if args.boot:
    print("boot")
    client.boot()


if __name__ == '__main__':
  main()

