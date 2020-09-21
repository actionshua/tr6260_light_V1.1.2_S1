import socket
import time
import random
import pysnooper

def get_crc_1byte(byte_in, crc_in=0):
    crc_out = crc_in
    crc_out ^= byte_in
    for i in range(8):
        if crc_out & 0x01:
            crc_out = (crc_out >> 1) ^ 0x8c
        else:
            crc_out >>= 1
    return crc_out


def get_crc_bytes(bytes_in):
    crc_out = 0
    for _byte in bytes_in:
        crc_out = get_crc_1byte(_byte, crc_out)
    return crc_out

# @pysnooper.snoop()
SMART_CONFIG_LEN_SET = ("total", "total_high", "total_low",
                        "ssid", "ssid_high", "ssid_low",
                        "password", "password_high", "password_low")

SMART_CONFIG_CRC_SET = ("ssid", "ssid_high", "ssid_low",
                        "password", "password_high", "password_low")

SMART_CONFIG_SEND_ROUND = 7
GUIDE_FIELD_CODE = ["a"*1024, "a"*1025, "a"*1026, "a"*1027]

GUIDE_FIELD_SEND_TIME = 2
GUIDE_FIELD_SEND_RATE = 0.010

MAGIC_CODE_SEND_RATE = 0.010
MAGIC_CODE_SEND_COUNT = (20*4)

PREFIX_CODE_SEND_RATE = 0.010
PREFIX_CODE_SEND_COUNT = 4
DATA_FIELD_SEND_ROUND = 5
DATA_FIELD_SEND_RATE = 0.010

SMART_CONFIG_RECV_ROUND = 7
SMART_CONFIG_SEND_TIME = 6

SMARTCONFIG_DA = ("224.0.0.76", 10000)

SMART_CONFIG_SOURCE_PORT = random.randint(10000, 11000)

@pysnooper.snoop()
def _calc_smart_config_len(total_len, ssid_len, password_len):
    len_dict = dict.fromkeys(SMART_CONFIG_LEN_SET)
    len_dict["total"] = total_len if total_len > 16 else total_len+128
    len_dict["total_high"] = ((len_dict["total"]) & 0xF0) >> 4
    len_dict["total_low"] = (len_dict["total"]) & 0x0F
    len_dict["ssid"] = ssid_len
    len_dict["ssid_high"] = ((len_dict["ssid"]) & 0xF0) >> 4
    len_dict["ssid_low"] = (len_dict["ssid"]) & 0x0F
    len_dict["password"] = password_len
    len_dict["password_high"] = ((len_dict["password"]) & 0xF0) >> 4
    len_dict["password_low"] = (len_dict["password"]) & 0x0F
    print len_dict
    return len_dict

@pysnooper.snoop()
def _calc_smart_config_crc(ssid, password_len):
    crc_dict = dict.fromkeys(SMART_CONFIG_CRC_SET)
    crc_dict["ssid"] = get_crc_bytes(ssid)
    crc_dict["ssid_high"] = ((crc_dict["ssid"]) & 0xF0) >> 4
    crc_dict["ssid_low"] = (crc_dict["ssid"]) & 0x0F
    crc_dict["password"] = get_crc_1byte(password_len)
    crc_dict["password_high"] = ((crc_dict["password"]) & 0xF0) >> 4
    crc_dict["password_low"] = (crc_dict["password"]) & 0x0F
    print crc_dict
    return crc_dict


def _smart_config_pre_process(ssid, password, ip="255.255.255.255",
                              bssid="ff:ff:ff:ff:ff:ff", is_hidden=0):
    ascii_ssid = [ord(char) for char in ssid]
    ascii_password = [ord(char) for char in password]

    data_buffer = list(ascii_password)
    data_buffer.append(random.randint(0, 255))
    data_buffer.extend(ascii_ssid)

    len_dict = _calc_smart_config_len(len(data_buffer), len(ascii_ssid), len(password))
    crc_dict = _calc_smart_config_crc(ascii_ssid, len_dict["password"])

    return data_buffer, len_dict, crc_dict
    pass


def _generate_control_field(len_dict, crc_dict):
    magic_code = ['m'*len_dict["total_high"],
                  'm'*((0x1 << 4) | len_dict["total_low"]),
                  'm'*((0x2 << 4) | crc_dict["ssid_high"]),
                  'm'*((0x3 << 4) | crc_dict["ssid_low"])]

    prefix_code = ['p'*((0x4 << 4) | len_dict["password_high"]),
                   'p'*((0x5 << 4) | len_dict["password_low"]),
                   'p'*((0x6 << 4) | crc_dict["password_high"]),
                   'p'*((0x7 << 4) | crc_dict["password_low"])]

    return GUIDE_FIELD_CODE, magic_code, prefix_code
    pass


def _smart_config_send_guide_code(sock, guide_code):
    # send guide field for 2s
    for i in range(int(GUIDE_FIELD_SEND_TIME/GUIDE_FIELD_SEND_RATE)):
        sock.sendto(guide_code[i % 4], SMARTCONFIG_DA)
        time.sleep(GUIDE_FIELD_SEND_RATE)
        pass
    pass


def _smart_config_send_magic_code(sock, magic_code):
    # send magic code
    for i in range(MAGIC_CODE_SEND_COUNT):
        sock.sendto(magic_code[i % 4], SMARTCONFIG_DA)
        time.sleep(MAGIC_CODE_SEND_RATE)
        pass
    pass


def _smart_config_send_prefix_code(sock, prefix_code):
    # send prefix code
    for i in range(PREFIX_CODE_SEND_COUNT):
        sock.sendto(prefix_code[i % 4], SMARTCONFIG_DA)
        time.sleep(PREFIX_CODE_SEND_RATE)
        pass


def _smartconfig_send(sock, data_buffer, guide_code, magic_code, prefix_code):
    # send guide code
    _smart_config_send_guide_code(sock, guide_code)
    # send magic code
    _smart_config_send_magic_code(sock, magic_code)

    for i in range(DATA_FIELD_SEND_ROUND):
        # prefix data send first
        _smart_config_send_prefix_code(sock, prefix_code)
        # then send data field
        seq_num = 0
        send_len = 0
        while send_len < len(data_buffer):
            
            data_items = 4 if (len(data_buffer) - send_len) >= 4 else (len(data_buffer) - send_len)
            _current_data_buffer = data_buffer[send_len:(send_len+data_items)]
            _current_data_buffer.insert(0, seq_num)

            data_crc = get_crc_bytes(_current_data_buffer)

            data_filed_len = [128+(data_crc & 0x7F)]
            data_filed_len.extend([256+_length for _length in _current_data_buffer])
            data_filed_len[1] -= 128

            data_frame = ["d" * data_len for data_len in data_filed_len]
            
            for _data in data_frame:
                sock.sendto(_data, SMARTCONFIG_DA)
                time.sleep(DATA_FIELD_SEND_RATE)

            send_len += data_items
            seq_num += 1
    pass



def do_smartconfig(ssid, password, sock, ip, bssid, is_hidden):
    # pre process
    data_buffer, len_dict, crc_dict = _smart_config_pre_process(ssid, password)
    # get codes
    guide_code, magic_code, prefix_code = _generate_control_field(len_dict, crc_dict)
    # send all fields
    for i in range(SMART_CONFIG_SEND_ROUND):
        _smartconfig_send(sock, data_buffer, guide_code, magic_code, prefix_code)
    pass


def main():
    # create socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(120)
    # bind to you PC Wifi NIC
    sock.bind(("0.0.0.0", SMART_CONFIG_SOURCE_PORT))
    do_smartconfig("TP-LINK_WDR7660", "12345678", sock, "192.168.1.100", "FF:FF:FF:FF:FF:FF", False)
    sock.close()
    pass

if __name__ == '__main__':
    main()

