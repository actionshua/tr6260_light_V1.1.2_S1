#!/usr/bin/env python
#

import sys
import struct
import serial
import time
import argparse
import os
import sys  
import binascii
import subprocess
import tempfile
import inspect


__version__ = "1.0.1"
# These are the currently known commands supported by the ROM

TRS_SYNC        = 0x73796E63
TRS_DEFAULT_ROM_BAUD = 57600
TRS_UBOOT_ADDR   = 0x0000
TRS_ROM_BUAD_57600  = 0x02
TRS_ROM_BUAD_115200 = 0x01
TRS_ROM_BUAD_380400 = 0x04
TRS_ROM_BUAD_460800 = 0x05
TRS_ROM_BUAD_ACK = 0x03
TRS_ROM_BUAD_ERR = 0xFF
TRS_FRM_TYPE_UBOOT = 0x00000001
TRS_FRM_TYPE_NV    = 0x00000002
TRS_FRM_TYPE_APP   = 0x00000003

class TRSROM:
    TRS_FLASH_BLOCK = 1024
    def __init__(self, port=0, baud = TRS_DEFAULT_ROM_BAUD):
        self.new_baudrate = baud
        self._port = serial.Serial(port)
        self._port.baudrate = TRS_DEFAULT_ROM_BAUD
        self.has_uboot = False  # actually unknown, but assume not
        self.baudrate_is_set = False
        self.download_uboot = False
        self.download_app = False

    """ Write bytes to the serial port"""
    def write(self, packet):
        self._port.write(packet)

    """ Send a request and read the response """
    def command(self, op=None, len = 0):
        if op is not None:
            if len == 4:
                pkt = struct.pack('<I', op)
            elif len == 1:
                pkt = struct.pack('<B', op)
            self.write(pkt)

    """ Receive a response to a command """
    def receive_response(self):
        # Read header of response and parse
        ret = self._port.read(1)
        if ret:
            resp = struct.unpack('<B', ret)
            return resp[0]
    
    """ Perform a connection test """
    def sync(self):
        retries = 100
        while retries > 0:
            self.command(TRS_SYNC, 4)
            time.sleep(0.05)
            resp = self.receive_response()
            retries = retries - 1
            # tries to get a response until that response has the
            # same operation as the request or a retries limit has
            # exceeded.
            if resp == 0x01:
                self.is_bootloader = True
                break
            elif resp == 0x02:
                self.is_bootloader = False
                break
            else:
                raise FatalError('Invalid response 0x%02x" to command' % resp)

    """ Baudrate config, set new baudrate """
    def baudrate_cfg(self):
        if self.new_baudrate == 115200:
            self.command(TRS_ROM_BUAD_115200, 1)
            self._port.baudrate = self.new_baudrate
            time.sleep(0.05)
            resp = self.receive_response()
            if resp is not TRS_ROM_BUAD_ACK:
                return False
            else:
                return True
        elif self.new_baudrate == 460800:
            self.command(TRS_ROM_BUAD_460800, 1)
            self._port.baudrate = self.new_baudrate
            time.sleep(0.05)
            resp = self.receive_response()
            if resp is not TRS_ROM_BUAD_ACK:
                return False
            else:
                return True
        else:
            self.command(TRS_ROM_BUAD_57600, 1)
            time.sleep(0.05)
            resp = self.receive_response()
            if resp is not TRS_ROM_BUAD_ACK:
                return False
            else:
                return True
            return True

    """ flash begin """
    def flash_begin(self, type, len, addr):
        try:
            num_blocks = (len + self.TRS_FLASH_BLOCK - 1) // self.TRS_FLASH_BLOCK

            image_cfg = struct.pack('<3I',type, addr, len)
            self.write(image_cfg)

            resp = None
            while resp == None:
                time.sleep(0.05)
                resp = self.receive_response()
            return num_blocks
        except FatalError as e:
            time.sleep(0.05)
            raise FatalError('Failed to erase flash')

    def run(self):
        try:
            run_cfg = struct.pack('<3I',0, 0, 0)
            self.write(run_cfg)
            time.sleep(0.05)
            resp = self.receive_response()
            return resp

        except FatalError as e:
            time.sleep(0.05)
            raise FatalError('Failed to erase flash')

    """ Try connecting repeatedly until successful, or giving up """
    def begin(self,address,file,ret):
        
        try:
            if ret == 0x01:
                self.has_uboot = False
                if address != TRS_UBOOT_ADDR:
                    raise FatalError ('not uboot in flash,please download uboot firstly')
                else:
                    image = file.read()
                    seq = 0
                    written = 0
                    t = time.time()
                    print ('will load uboot to ram firstly')
                    blocks = self.flash_begin(TRS_FRM_TYPE_UBOOT ,len(image), address)
                    time.sleep(0.05)
                    while len(image) > 0:
                        print '\rWriting at 0x%08x... (%d %%)' % (address + seq * self.TRS_FLASH_BLOCK, 100 * (seq + 1) / blocks),
                        sys.stdout.flush()
                        block = image[0:self.TRS_FLASH_BLOCK]
                        self.write(block)
                        time.sleep(0.05)
                        ret = self.receive_response()
                        if ret != 0x00:
                            raise FatalError('load ram error')
                        image = image[self.TRS_FLASH_BLOCK:]
                        seq += 1
                        written += len(block)
                    t = time.time() - t
                    print '\rload %d byte in %.1f seconds (%.1f kbit/s)...' % (written, t, written / t * 8 / 1000)
                    time.sleep(0.5)
                    self.command(TRS_SYNC, 4)
                    ret = self.receive_response()
            return ret
        except FatalError as e:
            time.sleep(0.05)
            raise FatalError('Failed to connect to TRS6260')

def arg_auto_int(x):
    return int(x, 0)

def write_flash(trs, args):
    trs._port.timeout = 2
    trs._port.flushInput()
    trs._port.flushOutput()
    trs.command(TRS_SYNC, 4)
    time.sleep(0.05)
    ret = trs.receive_response()
    
    for address, argfile in args.addr_filename:
        resp = trs.begin(address,argfile,ret)
        ret = resp
        if address == TRS_UBOOT_ADDR:
            frm_type = TRS_FRM_TYPE_UBOOT
        else:
            frm_type = TRS_FRM_TYPE_APP
        if resp == 0x02:
            print ('there is an uboot version in ram')
            trs.has_uboot = True
        else:
            raise FatalError('Failed to sync')
        
        if trs.baudrate_is_set == False and trs.has_uboot == True:
            resp = trs.baudrate_cfg()
            if resp is not True:
                raise FatalError('Failed to set baudrate')
            else:
                time.sleep(0.05)
                trs.command(TRS_SYNC, 4)

                time.sleep(0.05)
                resp = trs.receive_response()
                trs.baudrate_is_set = True

        argfile.seek(0)
        image = argfile.read()
        argfile.seek(0)  # in case we need it again
        print 'Erasing flash...'
        blocks = trs.flash_begin(frm_type ,len(image), address)
        time.sleep(0.05)
        seq = 0
        written = 0
        t = time.time()
        header_block = None
        while len(image) > 0:
            print '\rWriting at 0x%08x... (%d %%)' % (address + seq * trs.TRS_FLASH_BLOCK, 100 * (seq + 1) / blocks),
            sys.stdout.flush()
            block = image[0:trs.TRS_FLASH_BLOCK]
            # Pad the last block
            block = block + '\xff' * (trs.TRS_FLASH_BLOCK - len(block))
            trs.write(block)
            time.sleep(0.05)
            resp = trs.receive_response()
            if resp != 0x00:
                raise FatalError('write flash error')
            image = image[trs.TRS_FLASH_BLOCK:]
            seq += 1
            written += len(block)
        t = time.time() - t
        print '\rWrote %d bytes at 0x%08x in %.1f seconds (%.1f kbit/s)...' % (written, address, t, written / t * 8 / 1000)
    print '\nLeaving...'


def version(args):
    print __version__

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
        default=os.environ.get('TRSTOOL_BAUD', TRS_DEFAULT_ROM_BAUD))

    subparsers = parser.add_subparsers(
        dest='operation',
        help='Run trstool {command} -h for additional help')

    def add_spi_flash_subparsers(parent):
        """ Add common parser arguments for SPI flash properties """
        parent.add_argument('--flash_size', '-fs', help='SPI Flash size in Mbytes', type=str.lower,
                            choices=['512k', '1M', '2M', '4M'],
                            default=os.environ.get('TRSTOOL_FS', '4m'))

    parser_write_flash = subparsers.add_parser(
        'write_flash',
        help='Write a binary blob to flash')
    parser_write_flash.add_argument('addr_filename', metavar='<address> <filename>', help='Address followed by binary filename, separated by space',
                                    action=AddrFilenamePairAction)
    add_spi_flash_subparsers(parser_write_flash)
    parser_write_flash.add_argument('--verify', help='Verify just-written data (only necessary if very cautious, data is already CRCed', action='store_true')

    # todo
    subparsers.add_parser(
        'erase_flash',
        help='Perform Chip Erase on SPI flash')

    subparsers.add_parser(
        'version', help='Print trstool version')

    args = parser.parse_args()

    print 'trstool.py v%s' % __version__

    # operation function can take 1 arg (args), 2 args (trs, arg)
    # or be a member function of the TRSROM class.

    operation_func = globals()[args.operation]
    operation_args,_,_,_ = inspect.getargspec(operation_func)
    if operation_args[0] == 'trs':  # operation function takes an TRSROM connection object
        trs = TRSROM(args.port, args.baud)
        operation_func(trs, args)
    else:
        operation_func(args)
    #run the image
    trs.run()
    trs._port.close()


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

if __name__ == '__main__':
    try:
        main()
    except FatalError as e:
        print '\nA fatal error occurred: %s' % e
        sys.exit(2)

