#!/bin/python3
import argparse
import socket

path = "/tmp/mintd.socket";
mintd_magic = b'MT'

class Commands:
    ADD = 0x0
    QUERY_STATUS = 0x1

def send_packet(packet) -> bytes:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.connect(path)
        s.send(packet)
        data = s.recv(1024)
        return data

def create_status_packet() -> bytes:
    return mintd_magic + Commands.QUERY_STATUS.to_bytes(2, byteorder='little') + bytes(4)

def mintd_status(args):
    packet = create_status_packet();
    response = send_packet(packet)
    print(response.decode('utf-8'))

def create_add_packet(magnet_url) -> bytes:
    return mintd_magic + Commands.ADD.to_bytes(2, byteorder='little') + bytes(4) + magnet_url.encode('utf-8')

def mintd_add(args):
    packet = create_add_packet(args.magnet_url)
    response = send_packet(packet)
    print(response.decode('utf-8'))

def parse_cmd():
    parser = argparse.ArgumentParser(description='tiny client for mintd')
    subparser = parser.add_subparsers(dest='cmd', required=True)
    add_parser = subparser.add_parser('add')
    add_parser.add_argument('magnet_url')
    add_parser.set_defaults(func=mintd_add)
    #remove_parser = subparser.add_parser('remove')
    #remove_parser.add_argument('num')
    #remove_parser.set_defaults(func=mintd_add)
    status_parser = subparser.add_parser('status')
    status_parser.set_defaults(func=mintd_status)
    args = parser.parse_args()
    return args

def main():
    args = parse_cmd()
    args.func(args)

if __name__ == "__main__":
    main()
