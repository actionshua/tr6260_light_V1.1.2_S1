#!/usr/bin/env python
#coding:utf-8

# 组播包smartconfig算法：
# 1.有效信息存储在128字节的数组内，4字节ip+2字节port+password+0+ssid+0+2字节option+1字节异或校验
# 2.发送包的信息和上述的数组关系是：
# 组播包的第2,3,3地址对应了发送的bssid，将上述信息可以隐藏在组播IP地址的第2至4字节即可完成信息传递

import socket
import time
import random


SMART_CONFIG_SOURCE_PORT = random.randint(10000, 11000)
DATA_SEND_RATE = 0.01


def _smartconfig_broadcast_send(ssid, password, sock, ip, port):
    pass

def _smartconfig_mutilcast_send(ssid, password, sock, ip, port):
    ascii_ssid = [ord(char) for char in ssid]
    ascii_password = [ord(char) for char in password]
    data_buff = list()
    ip_bytes = ip.split('.')
    data_buff = ([int(ip_bytes[i]) for i in range(4)])
    data_buff.append(port&0xFF)
    data_buff.append((port>>8)&0xFF)
    data_buff.extend(ascii_password)
    data_buff.append(0)
    data_buff.extend(ascii_ssid)
    data_buff.append(0)
    data_buff.append(0x0020) #option
    
    all_xor = 0
    for data in data_buff:
        all_xor ^= data
    data_buff.append(all_xor)

    # print data_buff
    mt_ip_len_buf = "225.254.%d.170" % (len(data_buff))
    # print mt_ip_len_buf
    mt_ip_data_buf = []
    for i in range(len(data_buff) - 1):
        data_buf =  "225.%d.%d.%d" % (i, data_buff[i],data_buff[i+1])
        mt_ip_data_buf.append(data_buf)
    # print mt_ip_data_buf

    SCLEN_DA = (mt_ip_len_buf, port)
    SCDATA_DA = []
    for i in range(len(data_buff) - 1):
        SCDATA_DA.append((mt_ip_data_buf[i], port)) 
    
    
    _data = "0" * 1
    sock.sendto(_data, SCLEN_DA)
    for i in range(len(data_buff) - 1):
        # print SCDATA_DA
        sock.sendto(_data, SCDATA_DA[i])
        time.sleep(DATA_SEND_RATE)


def do_smartconfig(ssid, password, sock, ip, port):

    # do mutilcast 
    while True:
        _smartconfig_mutilcast_send(ssid, password,sock,ip,port)
        # _smartconfig_broadcast_send(ssid, password,sock,ip,port)
    
    pass



def main():
    # create socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(30)
    # bind to you PC ip
    sock.bind(("192.168.128.11", SMART_CONFIG_SOURCE_PORT))
    do_smartconfig("ESWIN-1", "eswinnj0", sock, "192.168.128.11",SMART_CONFIG_SOURCE_PORT)
    sock.close()
    pass

if __name__ == '__main__':
    main()
