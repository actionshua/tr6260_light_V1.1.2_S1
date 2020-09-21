#!/usr/bin/env python
#
from __future__ import division, print_function
import sys
import struct
import serial
import time
import argparse
import os
import sys
import zlib
import binascii
import subprocess
import tempfile
import inspect
import string
import base64


__version__ = "1.0.1"

# this is frame struct support by uboot
# ,-----+------+-----+----------+------------+- - - -+-------------,
# | SOF | TYPE | LEN | SUB_TYPE | HEAD_CKSUM | DATA  | DATA_CKSUM  |
# |  1  |   1  |  4  |    1     |      4     | ...   |     4       | <- size (bytes)
# '-----+------+-----+----------+------------+- - - -+-------------'

TRS_SYNC        = 0x73796E63
TRS_DEFAULT_ROM_BAUD = 57600
SOF = 0xA5 # start of a frame

# message type define
M_SYNC  = 0x00
M_CFG   = 0x01
M_WRITE = 0x02
M_READ  = 0x03
M_ERASE = 0x04

#Mesage_subtype define
M_SUB_SYNC      =  0x00
M_SUB_BAUD      =  0x01
M_SUB_FLASH     =  0x02
M_SUB_FLASH_ID  =  0x03
M_SUB_MEM       =  0x04
M_SUB_REG       =  0x05
M_SUB_VERSION   =  0x06

#response message
ACK_OK  = 0x00
ACK_ERR = 0x01

PYTHON2 = sys.version_info[0] < 3

# Function to return nth byte of a bitstring
# Different behaviour on Python 2 vs 3
if PYTHON2:
    def byte(bitstr, index):
        return ord(bitstr[index])
else:
    def byte(bitstr, index):
        return bitstr[index]

class TRSROM:
    TRS_STUB_BLOCK = 1024
    TRS_CMD_BLOCK = 4096
    def __init__(self, port=0, baud = TRS_DEFAULT_ROM_BAUD):
        self.new_baudrate = baud
        self._port = serial.Serial(port)
        self._port.baudrate = TRS_DEFAULT_ROM_BAUD

    """ Write bytes to the serial port"""
    def write(self, packet):
        self._port.write(packet)

    """ Read bytes from the serial port"""
    def read(self, bytes):
        return self._port.read(bytes)

    """ Send a request and read the response """
    def command(self, cmd_type=None, cmd_subtype=None,data_content=None):
        if (cmd_type and cmd_subtype) is not None:
            hdr_checksum = binascii.crc32(struct.pack('<BBIB', SOF,cmd_type,len(data_content),cmd_subtype),0x00000000) & 0xffffffff
            data_checksum = binascii.crc32(struct.pack("%ds" % (len(data_content)), data_content),0x00000000) & 0xffffffff
            pkt = struct.pack("<BBIBI%dsI" % (len(data_content)), SOF,cmd_type,len(data_content),cmd_subtype,hdr_checksum,data_content,data_checksum)
            # debug line
            # print (HexFormatter(pkt))
            self.write(pkt)
            resp = self.receive_response()
            if resp != 0x00:
                return False
            return True

    def read_mem(self, offset, load_length, progress_fn=None):
        length = load_length
        data = b''
        while length:
            if length < self.TRS_CMD_BLOCK:
                data_pkt = struct.pack('<II',offset,length)
                res = self.command(M_READ,M_SUB_MEM,data_pkt)
                if res != True:
                    raise FatalError('Failed to read mem')
                p = self.read(length)
                data += p
                length -= length
                ret = self.receive_response()
            else:
                data_pkt = struct.pack('<II',offset,self.TRS_CMD_BLOCK)
                res = self.command(M_READ,M_SUB_MEM,data_pkt)
                if res != True:
                    raise FatalError('Failed to read mem')
                p = self.read(self.TRS_CMD_BLOCK)
                data += p
                length -= self.TRS_CMD_BLOCK

            if progress_fn and (len(data) % 1024 == 0 or len(data) == load_length):
                progress_fn(len(data), load_length)
        if progress_fn:
            progress_fn(len(data), load_length)
        if len(data) > load_length:
            raise FatalError('Read more than expected')
        return data

    def read_flash(self, offset, image_length, progress_fn=None):
        length = image_length
        data = b''
        while length:
            if length < self.TRS_CMD_BLOCK:
                data_pkt = struct.pack('<II',offset,length)
                res = self.command(M_READ,M_SUB_FLASH,data_pkt)
                if res != True:
                    raise FatalError('Failed to read flash')
                p = self.read(length)
                data += p
                length -= length
                ret = self.receive_response()
            else:
                data_pkt = struct.pack('<II',offset,self.TRS_CMD_BLOCK)
                res = self.command(M_READ,M_SUB_FLASH,data_pkt)
                if res != True:
                    raise FatalError('Failed to read flash')
                p = self.read(self.TRS_CMD_BLOCK)
                data += p
                length -= self.TRS_CMD_BLOCK

            if progress_fn and (len(data) % 1024 == 0 or len(data) == image_length):
                progress_fn(len(data), image_length)
        if progress_fn:
            progress_fn(len(data), image_length)
        if len(data) > image_length:
            raise FatalError('Read more than expected')
        return data

    """ Receive a response to a command """
    def receive_response(self):
        # Read header of response and parse
        ret = self._port.read(1)
        if ret:
            resp = struct.unpack('<B', ret)
            return resp[0]

    """ Perform a connection to bootrom in uart boot mode """
    def sync_bootrom(self):
        self._port.timeout = 5
        self._port.flushInput()
        self._port.flushOutput()
        retries = 100
        while retries > 0:
            self.write(struct.pack('<I', TRS_SYNC))
            print('Connecting...')
            sys.stdout.flush()
            time.sleep(0.1)
            resp = self.receive_response()
            retries = retries - 1
            # tries to get a response until that response has the
            # same operation as the request or a retries limit has
            # exceeded.
            if resp == 0x01:
                break
        if retries <= 0:
            raise FatalError('Failed to connect to target')

    """ Try loading stub code to ram and run it """
    def load_stub(self):
        try:
            seq = 0
            image = self.STUB_CODE
            written = 0
            t = time.time()
            print ('will load stub to ram firstly')
            num_blocks = (len(image) + self.TRS_STUB_BLOCK - 1) // self.TRS_STUB_BLOCK
            image_cfg = struct.pack('<3I',0x00000001, 0, len(image))
            self.write(image_cfg)
            resp = None
            while resp == None:
                time.sleep(0.05)
                resp = self.receive_response()
            while len(image) > 0:
                print('\rWriting at 0x%08x... (%d %%)' % (seq * self.TRS_STUB_BLOCK, 100 * (seq + 1) / num_blocks),end='')
                sys.stdout.flush()
                block = image[0:self.TRS_STUB_BLOCK]
                self.write(block)
                # print (HexFormatter(block))
                ret = self.receive_response()
                if ret != 0x00:
                    raise FatalError('load stub error')
                image = image[self.TRS_STUB_BLOCK:]
                seq += 1
                written += len(block)
            t = time.time() - t
            print ('\rload %d byte in %.1f seconds (%.1f kbit/s)...' % (written, t, written / t * 8 / 1000))
            time.sleep(0.05)
        except FatalError as e:
            time.sleep(0.05)
            raise FatalError('Failed to loading stub to TR6260')


def arg_auto_int(x):
    return int(x, 0)

def hexify(s, uppercase=True):
    format_str = '%02X' if uppercase else '%02x'
    if not PYTHON2:
        return ''.join(format_str % c for c in s)
    else:
        return ''.join(format_str % ord(c) for c in s)

class HexFormatter(object):
    """
    Wrapper class which takes binary data in its constructor
    and returns a hex string as it's __str__ method.

    This is intended for "lazy formatting" of trace() output
    in hex format. Avoids overhead (significant on slow computers)
    of generating long hex strings even if tracing is disabled.

    Note that this doesn't save any overhead if passed as an
    argument to "%", only when passed to trace()

    If auto_split is set (default), any long line (> 16 bytes) will be
    printed as separately indented lines, with ASCII decoding at the end
    of each line.
    """
    def __init__(self, binary_string, auto_split=True):
        self._s = binary_string
        self._auto_split = auto_split

    def __str__(self):
        if self._auto_split and len(self._s) > 16:
            result = ""
            s = self._s
            while len(s) > 0:
                line = s[:16]
                ascii_line = "".join(c if (c == ' ' or (c in string.printable and c not in string.whitespace))
                                     else '.' for c in line.decode('ascii', 'replace'))
                s = s[16:]
                result += "\n    %-16s %-16s | %s" % (hexify(line[:8], False), hexify(line[8:], False), ascii_line)
            return result
        else:
            return hexify(self._s, False)

def pad_to(data, alignment, pad_character=b'\xFF'):
    """ Pad to the next alignment boundary """
    pad_mod = len(data) % alignment
    if pad_mod != 0:
        data += pad_character * (alignment - pad_mod)
    return data

def erase_region(trs, args):
    if args.address % 4096 != 0 or args.size % 4096 != 0:
        raise FatalError('erase flash start address and size must be multiple of 4096')
    trs.sync_bootrom()
    trs.load_stub()
    sync_data = 0x00
    data_pkt = struct.pack('<B',sync_data)
    sync_res = trs.command(M_SYNC,M_SUB_SYNC,data_pkt)
    if sync_res == False:
        raise FatalError('Failed to sync to TR6260')
    if trs.new_baudrate != 57600:
        multiple_baud = (trs.new_baudrate/57600)
        data_pkt = struct.pack('<B',multiple_baud)
        res = trs.command(M_CFG,M_SUB_BAUD,data_pkt)
        if res is True:
            trs._port.baudrate = trs.new_baudrate
            time.sleep(0.05)
        else:
            raise FatalError('Failed to modify baudrate')
    t = time.time()
    trs._port.timeout = 30  # because erase a flash region need a lot of time
    data_pkt = struct.pack('<II',args.address,args.size)
    res = trs.command(M_ERASE,M_SUB_FLASH,data_pkt)
    if res != True:
        raise FatalError('Failed to erase flash')
    t = time.time() - t
    trs._port.timeout = 5
    print ('Erase region from 0x%08x to 0x%08x in %.1f seconds...' % (args.address, args.address+args.size, t))

def erase_flash(trs, args):
    print ('flash chip erase don\'t support now! but in our \'TODO\' list,sorry about that')


def read_flash(trs, args):
    trs.sync_bootrom()
    trs.load_stub()
    sync_data = 0x00
    data_pkt = struct.pack('<B',sync_data)
    sync_res = trs.command(M_SYNC,M_SUB_SYNC,data_pkt)
    if sync_res == False:
        raise FatalError('Failed to sync to TR6260')
    if trs.new_baudrate != 57600:
        multiple_baud = (trs.new_baudrate/57600)
        data_pkt = struct.pack('<B',multiple_baud)
        res = trs.command(M_CFG,M_SUB_BAUD,data_pkt)
        if res is True:
            trs._port.baudrate = trs.new_baudrate
            time.sleep(0.05)
        else:
            raise FatalError('Failed to modify baudrate')

    def flash_progress(progress, length):
        msg = '%d (%d %%)' % (progress, progress * 100.0 / length)
        padding = '\b' * len(msg)
        if progress == length:
            padding = '\n'
        sys.stdout.write(msg + padding)
        sys.stdout.flush()
    t = time.time()
    data = trs.read_flash(args.address, args.size, flash_progress)
    t = time.time() - t
    print('\rRead %d bytes at 0x%x in %.1f seconds (%.1f kbit/s)...'
          % (len(data), args.address, t, len(data) / t * 8 / 1000))
    open(args.filename, 'wb').write(data)

def write_flash(trs, args):
    trs.sync_bootrom()
    trs.load_stub()
    sync_data = 0x00
    data_pkt = struct.pack('<B',sync_data)
    sync_res = trs.command(M_SYNC,M_SUB_SYNC,data_pkt)
    if sync_res == False:
        raise FatalError('Failed to sync to TR6260')
    if trs.new_baudrate != 57600:
        multiple_baud = (trs.new_baudrate/57600)
        data_pkt = struct.pack('<B',multiple_baud)
        res = trs.command(M_CFG,M_SUB_BAUD,data_pkt)
        trs._port.timeout = 10
        if res is True:
            trs._port.baudrate = trs.new_baudrate
            time.sleep(0.05)
        else:
            raise FatalError('Failed to modify baudrate')
    for address, argfile in args.addr_filename:
        argfile.seek(0)
        image = argfile.read()
        argfile.seek(0)  # in case we need it again
        print ('Erasing flash...')
        data_pkt = struct.pack('<II',address,len(image))
        res = trs.command(M_ERASE,M_SUB_FLASH,data_pkt)
        if res is not True:
           raise FatalError('Failed to erase flash')
        seq = 0
        written = 0
        t = time.time()
        num_blocks = (len(image) + trs.TRS_CMD_BLOCK - 1) // trs.TRS_CMD_BLOCK
        while len(image) > 0:
            print ('\rWriting at 0x%08x... (%d %%)' % (address + seq * trs.TRS_CMD_BLOCK, 100 * (seq + 1) / num_blocks),end='')
            sys.stdout.flush()
            block = image[0:trs.TRS_CMD_BLOCK]
            # Pad the last block
            block = block + '\xff' * (trs.TRS_CMD_BLOCK - len(block))
            data_pkt = struct.pack('<II',address,len(block))
            data_pkt += block
            address += len(block)
            res = trs.command(M_WRITE,M_SUB_FLASH,data_pkt)
            if res is not True:
               raise FatalError('Failed to write flash')
            image = image[trs.TRS_CMD_BLOCK:]
            seq += 1
            written += len(block)
        t = time.time() - t
        print ('\rWrote %d bytes at 0x%08x in %.1f seconds (%.1f kbit/s)...' % (written, address, t, written / t * 8 / 1000))
    print ('\nLeaving...')

def read_mem(trs, args):
    trs.sync_bootrom()
    trs.load_stub()
    sync_data = 0x00
    data_pkt = struct.pack('<B',sync_data)
    sync_res = trs.command(M_SYNC,M_SUB_SYNC,data_pkt)
    if sync_res == False:
        raise FatalError('Failed to sync to TR6260')
    if trs.new_baudrate != 57600:
        multiple_baud = (trs.new_baudrate/57600)
        data_pkt = struct.pack('<B',multiple_baud)
        res = trs.command(M_CFG,M_SUB_BAUD,data_pkt)
        if res is True:
            trs._port.baudrate = trs.new_baudrate
            time.sleep(0.05)
        else:
            raise FatalError('Failed to modify baudrate')
    def mem_progress(progress, length):
        msg = '%d (%d %%)' % (progress, progress * 100.0 / length)
        padding = '\b' * len(msg)
        if progress == length:
            padding = '\n'
        sys.stdout.write(msg + padding)
        sys.stdout.flush()
    t = time.time()
    data = trs.read_mem(args.address, args.size, mem_progress)
    t = time.time() - t
    print('\rRead %d bytes at 0x%x in %.1f seconds (%.1f kbit/s)...'
          % (len(data), args.address, t, len(data) / t * 8 / 1000))
    open(args.filename, 'wb').write(data)

def write_mem(trs, args):
    trs.sync_bootrom()
    trs.load_stub()
    sync_data = 0x00
    data_pkt = struct.pack('<B',sync_data)
    sync_res = trs.command(M_SYNC,M_SUB_SYNC,data_pkt)
    if sync_res == False:
        raise FatalError('Failed to sync to TR6260')
    if trs.new_baudrate != 57600:
        multiple_baud = (trs.new_baudrate/57600)
        data_pkt = struct.pack('<B',multiple_baud)
        res = trs.command(M_CFG,M_SUB_BAUD,data_pkt)
        if res is True:
            trs._port.baudrate = trs.new_baudrate
            time.sleep(0.05)
        else:
            raise FatalError('Failed to modify baudrate')

    for address, argfile in args.addr_filename:
        argfile.seek(0)
        mem_data = argfile.read()
        argfile.seek(0)  # in case we need it again
        seq = 0
        written = 0
        t = time.time()
        num_blocks = (len(mem_data) + trs.TRS_CMD_BLOCK - 1) // trs.TRS_CMD_BLOCK
        while len(mem_data) > 0:
            print ('\rWriting at 0x%08x... (%d %%)' % (address + seq * trs.TRS_CMD_BLOCK, 100 * (seq + 1) / num_blocks),end='')
            sys.stdout.flush()
            block = mem_data[0:trs.TRS_CMD_BLOCK]
            data_pkt = struct.pack('<II',address,len(block))
            data_pkt += block
            res = trs.command(M_WRITE,M_SUB_MEM,data_pkt)
            mem_data = mem_data[trs.TRS_CMD_BLOCK:]
            seq += 1
            written += len(block)
        t = time.time() - t
        print ('\rWrote %d bytes at 0x%08x in %.1f seconds (%.1f kbit/s)...' % (written, address, t, written / t * 8 / 1000))
    print ('\nLeaving...')

def read_reg(trs, args):
    trs.sync_bootrom()
    trs.load_stub()
    sync_data = 0x00
    data_pkt = struct.pack('<B',sync_data)
    sync_res = trs.command(M_SYNC,M_SUB_SYNC,data_pkt)
    if sync_res == False:
        raise FatalError('Failed to sync to TR6260')
    if trs.new_baudrate != 57600:
        multiple_baud = (trs.new_baudrate/57600)
        data_pkt = struct.pack('<B',multiple_baud)
        res = trs.command(M_CFG,M_SUB_BAUD,data_pkt)
        if res is True:
            trs._port.baudrate = trs.new_baudrate
            time.sleep(0.05)
        else:
            raise FatalError('Failed to modify baudrate')
    data_pkt = struct.pack('<II',args.address,0)
    res = trs.command(M_READ,M_SUB_REG,data_pkt)
    reg = trs.read(4)
    print ('reg %08x:%s' %(args.address,HexFormatter(reg)))

def write_reg(trs, args):
    trs.sync_bootrom()
    trs.load_stub()
    sync_data = 0x00
    data_pkt = struct.pack('<B',sync_data)
    sync_res = trs.command(M_SYNC,M_SUB_SYNC,data_pkt)
    if sync_res == False:
        raise FatalError('Failed to sync to TR6260')
    if trs.new_baudrate != 57600:
        multiple_baud = (trs.new_baudrate/57600)
        data_pkt = struct.pack('<B',multiple_baud)
        res = trs.command(M_CFG,M_SUB_BAUD,data_pkt)
        if res is True:
            trs._port.baudrate = trs.new_baudrate
            time.sleep(0.05)
        else:
            raise FatalError('Failed to modify baudrate')
    data_pkt = struct.pack('<II',args.address,arg.value)
    res = trs.command(M_WRITE,M_SUB_REG,data_pkt)
    if res != True:
        raise FatalError('Failed to write register %08x' % (args.address))

def flash_id(trs, arg):
    trs.sync_bootrom()
    trs.load_stub()
    sync_data = 0x00
    data_pkt = struct.pack('<B',sync_data)
    sync_res = trs.command(M_SYNC,M_SUB_SYNC,data_pkt)
    if sync_res == False:
        raise FatalError('Failed to sync to TR6260')
    if trs.new_baudrate != 57600:
        multiple_baud = (trs.new_baudrate/57600)
        data_pkt = struct.pack('<B',multiple_baud)
        res = trs.command(M_CFG,M_SUB_BAUD,data_pkt)
        if res is True:
            trs._port.baudrate = trs.new_baudrate
            time.sleep(0.05)
        else:
            raise FatalError('Failed to modify baudrate')

    data_pkt = struct.pack('<BH',0,0)
    res = trs.command(M_READ,M_SUB_FLASH_ID,data_pkt)
    if res != True:
        raise FatalError('Failed to read flash id register')
    else:
        res = trs.read(4)
        print ("flash id is %s" % (HexFormatter(res)))

class ImageSegment(object):
    """ Wrapper class for a segment in an TRS image
    (very similar to a section in an ELFImage also) """
    def __init__(self, addr, data, file_offs=None):
        self.addr = addr
        self.data = data
        self.file_offs = file_offs
        self.include_in_checksum = True
        if self.addr != 0:
            self.pad_to_alignment(4)  # pad all "real" ImageSegments 4 byte aligned length

    def copy_with_new_addr(self, new_addr):
        """ Return a new ImageSegment with same data, but mapped at
        a new address. """
        return ImageSegment(new_addr, self.data, 0)

    def split_image(self, split_len):
        """ Return a new ImageSegment which splits "split_len" bytes
        from the beginning of the data. Remaining bytes are kept in
        this segment object (and the start address is adjusted to match.) """
        result = copy.copy(self)
        result.data = self.data[:split_len]
        self.data = self.data[split_len:]
        self.addr += split_len
        self.file_offs = None
        result.file_offs = None
        return result

    def __repr__(self):
        r = "len 0x%05x load 0x%08x" % (len(self.data), self.addr)
        if self.file_offs is not None:
            r += " file_offs 0x%08x" % (self.file_offs)
        return r

    def pad_to_alignment(self, alignment):
        self.data = pad_to(self.data, alignment, b'\x00')

class ELFSection(ImageSegment):
    """ Wrapper class for a section in an ELF image, has a section
    name as well as the common properties of an ImageSegment. """
    def __init__(self, name, addr, data):
        super(ELFSection, self).__init__(addr, data)
        self.name = name.decode("utf-8")

    def __repr__(self):
        return "%s %s" % (self.name, super(ELFSection, self).__repr__())

class ELFFile(object):
    SEC_TYPE_PROGBITS = 0x01
    SEC_TYPE_STRTAB = 0x03

    LEN_SEC_HEADER = 0x28

    def __init__(self, name):
        # Load sections from the ELF file
        self.name = name
        with open(self.name, 'rb') as f:
            self._read_elf_file(f)

    def get_section(self, section_name):
        for s in self.sections:
            if s.name == section_name:
                return s
        raise ValueError("No section %s in ELF file" % section_name)

    def _read_elf_file(self, f):
        # read the ELF file header
        LEN_FILE_HEADER = 0x34
        try:
            (ident,_type,machine,_version,
             self.entrypoint,_phoff,shoff,_flags,
             _ehsize, _phentsize,_phnum, shentsize,
             shnum,shstrndx) = struct.unpack("<16sHHLLLLLHHHHHH", f.read(LEN_FILE_HEADER))
        except struct.error as e:
            raise FatalError("Failed to read a valid ELF header from %s: %s" % (self.name, e))

        if byte(ident, 0) != 0x7f or ident[1:4] != b'ELF':
            raise FatalError("%s has invalid ELF magic header" % self.name)
        if machine != 0xa7:
            raise FatalError("%s does not appear to be an Andes ELF file. e_machine=%04x" % (self.name, machine))
        if shentsize != self.LEN_SEC_HEADER:
            raise FatalError("%s has unexpected section header entry size 0x%x (not 0x28)" % (self.name, shentsize, self.LEN_SEC_HEADER))
        if shnum == 0:
            raise FatalError("%s has 0 section headers" % (self.name))
        self._read_sections(f, shoff, shnum, shstrndx)

    def _read_sections(self, f, section_header_offs, section_header_count, shstrndx):
        f.seek(section_header_offs)
        len_bytes = section_header_count * self.LEN_SEC_HEADER
        section_header = f.read(len_bytes)
        if len(section_header) == 0:
            raise FatalError("No section header found at offset %04x in ELF file." % section_header_offs)
        if len(section_header) != (len_bytes):
            raise FatalError("Only read 0x%x bytes from section header (expected 0x%x.) Truncated ELF file?" % (len(section_header), len_bytes))

        # walk through the section header and extract all sections
        section_header_offsets = range(0, len(section_header), self.LEN_SEC_HEADER)

        def read_section_header(offs):
            name_offs,sec_type,_flags,lma,sec_offs,size = struct.unpack_from("<LLLLLL", section_header[offs:])
            return (name_offs, sec_type, lma, size, sec_offs)
        all_sections = [read_section_header(offs) for offs in section_header_offsets]
        prog_sections = [s for s in all_sections if s[1] == ELFFile.SEC_TYPE_PROGBITS]

        # search for the string table section
        if not (shstrndx * self.LEN_SEC_HEADER) in section_header_offsets:
            raise FatalError("ELF file has no STRTAB section at shstrndx %d" % shstrndx)
        _,sec_type,_,sec_size,sec_offs = read_section_header(shstrndx * self.LEN_SEC_HEADER)
        if sec_type != ELFFile.SEC_TYPE_STRTAB:
            print('WARNING: ELF file has incorrect STRTAB section type 0x%02x' % sec_type)
        f.seek(sec_offs)
        string_table = f.read(sec_size)

        # build the real list of ELFSections by reading the actual section names from the
        # string table section, and actual data for each section from the ELF file itself
        def lookup_string(offs):
            raw = string_table[offs:]
            return raw[:raw.index(b'\x00')]

        def read_data(offs,size):
            f.seek(offs)
            return f.read(size)

        prog_sections = [ELFSection(lookup_string(n_offs), lma, read_data(offs, size)) for (n_offs, _type, lma, size, offs) in prog_sections
                         if lma != 0]
        self.sections = prog_sections

class TrsFirmwareImage():
    def __init__(self,arg,elf):
        UBOOT_HEAD_LEN = 0x40
        self.segments = elf.sections
        self.data = ""
        self.boot_size = 0
        normal_segments = self.segments
        for segment in normal_segments:
            self.data += segment.data
            self.boot_size += len(segment.data)

        self.boot_load = UBOOT_HEAD_LEN
        self.boot_ep = elf.entrypoint
        self.boot_name = "boot-version-debug"
        if arg.crc_enable == 'False':
            self.IMAGE_MAGIC = 0x02262018
            head_crc = 0
            data_crc = 0
        else:
            self.IMAGE_MAGIC = 0x48787032
            self.boot_name = self.boot_name.ljust(32, '\0')
            no_crc_header_data = struct.pack('>IIIIIIIBBBB32s',self.IMAGE_MAGIC, 0, 0, self.boot_size,\
            self.boot_load,self.boot_ep,0,0,0,0,0,self.boot_name)
            head_crc = zlib.crc32(no_crc_header_data, 0)
            # zlib.crc32
            data_crc = zlib.crc32(self.data, 0)


        self.uboot_head = struct.pack('>IIIIIIIBBBB32s',self.IMAGE_MAGIC, head_crc&0xFFFFFFFF, 0, self.boot_size,\
            self.boot_load,self.boot_ep,data_crc&0xFFFFFFFF,0,0,0,0,self.boot_name)

    def default_output_name(self, input_file):
        """ Derive a default output name from the ELF name. """
        return input_file + '-'

    """
    Boot header struct veryfied by boot rom:
    unsigned int    boot_magic;            /* Image Header Magic Number    */
    unsigned int    boot_hcrc;             /* Image Header CRC Checksum    */
    unsigned int    boot_time;             /* Image Creation Timestamp     */
    unsigned int    boot_size;             /* Image Data Size              */
    unsigned int    boot_load;             /* Data  Load  Address          */
    unsigned int    boot_ep;               /* Entry Point Address          */
    unsigned int    boot_dcrc;             /* Image Data CRC Checksum      */
    unsigned char   boot_os;               /* Operating System             */
    unsigned char   boot_arch;             /* CPU architecture             */
    unsigned char   boot_type;             /* Image Type                   */
    unsigned char   boot_comp;             /* Compression Type             */
    unsigned char   boot_name[32];         /* Image Name                   */

    but (boot_time,boot_os,boot_arch,boot_type,boot_comp,boot_name)
    has't been used by rom code now, so maybe is can be stored with some useful
    informations in follow-up bootloader version
    """

    def write_common_header(self, f, segments):
        f.write(self.uboot_head)

    """ Save the next segment to the image file """
    def save_segment(self, f, segment):
        f.write(segment.data)

    def save(self, basename):
        normal_segments = self.segments
        with open("%s" % basename, 'wb') as f:
            self.write_common_header(f, normal_segments)
            for segment in normal_segments:
                self.save_segment(f, segment)

def elf2image(args):
    e = ELFFile(args.input)
    image = TrsFirmwareImage(args,e);
    print ("bootloader section info:")
    print (e.sections)
    if args.output is None:
        args.output = image.default_output_name(args.input)
    image.save(args.output)


def version(args):
    print (__version__)

#
# End of operations functions
#

class FatalError(RuntimeError):
    """
    Wrapper class for runtime errors that aren't caused by internal bugs, but by
    TR6260 responses or input content.
    """
    def __init__(self, message):
        RuntimeError.__init__(self, message)

    @staticmethod
    def WithResult(message, result):
        """
        Return a fatal error object that includes the hex values of
        'result' as a string formatted argument.
        """
        return FatalError(message % ", ".join(hex(ord(x)) for x in result))


def main():
    parser = argparse.ArgumentParser(description='trstool.py v%s - TRS6260 ROM Bootloader Utility' % __version__, prog='trstool')

    parser.add_argument(
        '--port', '-p',
        help='Serial port device',
        default=os.environ.get('TRSTOOL_PORT', '/dev/ttyUSB0'))

    parser.add_argument(
        '--baud', '-b',
        help='Serial port baud rate',
        type=arg_auto_int,
        choices=[57600,115200,230400,460800,576000],
        default=os.environ.get('TRSTOOL_BAUD', TRS_DEFAULT_ROM_BAUD))

    subparsers = parser.add_subparsers(
        dest='operation',
        help='Run trstool {command} -h for additional help')

    def add_spi_flash_subparsers(parent, is_genimage):
        """ Add common parser arguments for SPI flash properties """
        extra_keep_args = [] if is_genimage else ['keep']
        auto_detect = not is_genimage

        parent.add_argument('--flash_freq', '-ff', help='SPI Flash frequency',
                            choices=extra_keep_args + ['40m', '26m', '20m', '80m'],
                            default=os.environ.get('TRSTOOL_FF', '40m' if is_genimage else 'keep'))
        parent.add_argument('--flash_mode', '-fm', help='SPI Flash mode',
                            choices=extra_keep_args + ['qio', 'qout', 'dio', 'dout'],
                            default=os.environ.get('TRSTOOL_FM', 'qio' if is_genimage else 'keep'))
        parent.add_argument('--flash_size', '-fs', help='SPI Flash size in MegaBytes (1MB, 2MB, 4MB, 8MB, 16M)',
                            default=os.environ.get('TRSTOOL_FS', 'detect' if auto_detect else '1MB'))

    parser_elf2image = subparsers.add_parser(
        'elf2image',
        help='Generate uboot binary file from input file')
    parser_elf2image.add_argument('input', help='Input original uboot file', type=str)
    parser_elf2image.add_argument('--crc_enable', '--crc', help='enable crc check for uboot of rom code',choices=['True','False'], default='False')
    parser_elf2image.add_argument('output', help='Output filename prefix', type=str)

    add_spi_flash_subparsers(parser_elf2image, is_genimage=True)

    parser_write_flash = subparsers.add_parser(
        'write_flash',
        help='Write a binary blob to flash')
    parser_write_flash.add_argument('addr_filename', metavar='<address> <filename>', help='Address followed by binary filename, separated by space',
                                    action=AddrFilenamePairAction)
    add_spi_flash_subparsers(parser_write_flash, is_genimage=False)
    parser_write_flash.add_argument('--verify', help='Verify just-written data (only necessary if very cautious, data is already CRCed', action='store_true')

    parser_read_flash = subparsers.add_parser(
        'read_flash',
        help='Read SPI flash content')
    add_spi_flash_subparsers(parser_read_flash, is_genimage=False)
    parser_read_flash.add_argument('address', help='Start address', type=arg_auto_int)
    parser_read_flash.add_argument('size', help='Size of region to dump', type=arg_auto_int)
    parser_read_flash.add_argument('filename', help='Name of binary dump')
    parser_read_flash.add_argument('--no-progress', '-p', help='Suppress progress output', action="store_true")

    parser_erase_flash = subparsers.add_parser(
        'erase_flash',
        help='Perform Chip Erase on SPI flash')
    add_spi_flash_subparsers(parser_erase_flash, is_genimage=False)

    parser_erase_region = subparsers.add_parser(
        'erase_region',
        help='Erase a region of the flash')
    add_spi_flash_subparsers(parser_erase_region, is_genimage=False)
    parser_erase_region.add_argument('address', help='Start address (must be multiple of 4096)', type=arg_auto_int)
    parser_erase_region.add_argument('size', help='Size of region to erase (must be multiple of 4096)', type=arg_auto_int)

    parser_read_mem = subparsers.add_parser(
        'read_mem',
        help='Read arbitrary memory location')
    parser_read_mem.add_argument('address', help='Address to read', type=arg_auto_int)
    parser_read_mem.add_argument('size', help='Size of region to dump', type=arg_auto_int)
    parser_read_mem.add_argument('filename', help='Name of binary dump')

    parser_write_mem = subparsers.add_parser(
        'write_mem',
        help='load data to arbitrary memory location')
    parser_write_mem.add_argument('addr_filename', metavar='<address> <filename>', help='Address followed by binary filename, separated by space',
            action=AddrFilenamePairAction)

    parser_read_reg = subparsers.add_parser(
        'read_reg',
        help='Read register value')
    parser_read_reg.add_argument('address', help='Address to read', type=arg_auto_int)

    parser_write_reg = subparsers.add_parser(
        'write_reg',
        help='Write register with given value')
    parser_write_reg.add_argument('address', help='Address to write', type=arg_auto_int)
    parser_write_reg.add_argument('value', help='Value', type=arg_auto_int)

    parser_write_reg = subparsers.add_parser(
        'flash_id',
         help='Read SPI flash manufacturer and device ID')

    subparsers.add_parser(
        'version', help='Print trstool version')

    args = parser.parse_args()

    print ('trstool.py v%s' % __version__)

    # operation function can take 1 arg (args), 2 args (trs, arg)
    # or be a member function of the TRSROM class.

    operation_func = globals()[args.operation]
    operation_args,_,_,_ = inspect.getargspec(operation_func)
    if operation_args[0] == 'trs':  # operation function takes an TRSROM connection object
        trs = TRSROM(args.port, args.baud)
        operation_func(trs, args)
        #run the image
        # trs.run()
        trs._port.close()
    else:
        operation_func(args)



class AddrFilenamePairAction(argparse.Action):
    """ Custom parser class for the address/filename pairs passed as arguments """
    def __init__(self, option_strings, dest, nargs='+', **kwargs):
        super(AddrFilenamePairAction, self).__init__(option_strings, dest, nargs, **kwargs)

    def __call__(self, parser, namtrsace, values, option_string=None):
        # validate pair arguments
        pairs = []
        for i in range(0,len(values),2):
            try:
                address = int(values[i],0)
            except ValueError as e:
                raise argparse.ArgumentError(self,'Address "%s" must be a number' % values[i])
            try:
                argfile = open(values[i + 1], 'rb')
            except IOError as e:
                raise argparse.ArgumentError(self, e)
            except IndexError:
                raise argparse.ArgumentError(self,'Must be pairs of an address and the binary filename to write there')
            pairs.append((address, argfile))
        setattr(namtrsace, self.dest, pairs)

TRSROM.STUB_CODE = base64.b64decode(b"""
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\
""")


if __name__ == '__main__':
    try:
        main()
    except FatalError as e:
        print ('\nA fatal error occurred: %s' % e)
        sys.exit(2)

