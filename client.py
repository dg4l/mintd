#!/bin/python3
import argparse
import socket

path = "/tmp/mintd.socket";
mintd_magic = b'MT'

class Commands:
    ADD = 0x0
    QUERY_STATUS = 0x1
    PAUSE_ALL = 0x2
    PAUSE_IDX = 0x3
    RESUME_ALL = 0x4
    RESUME_IDX = 0x5
    REMOVE_TORRENT = 0x6
    QUERY_STATS = 0x7
    QUERY_STATS_ALLTIME = 0x8


def send_packet(packet) -> bytes:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.connect(path)
        s.send(packet)
        data = s.recv(1024)
        return data

# lots of repeated logic but it doesn't matter for now
def create_pause_all_packet() -> bytes:
    return mintd_magic + Commands.PAUSE_ALL.to_bytes(2) + bytes(4)

def create_resume_all_packet() -> bytes:
    return mintd_magic + Commands.RESUME_ALL.to_bytes(2) + bytes(4)

def create_query_stats_packet() -> bytes:
    return mintd_magic + Commands.QUERY_STATS.to_bytes(2) + bytes(4)

def create_pause_packet(idx) -> bytes:
    return mintd_magic + Commands.PAUSE_IDX.to_bytes(2) + int(idx).to_bytes(4, signed=False)

def create_resume_packet(idx) -> bytes:
    return mintd_magic + Commands.RESUME_IDX.to_bytes(2) + int(idx).to_bytes(4, signed=False)

def create_status_packet() -> bytes:
    return mintd_magic + Commands.QUERY_STATUS.to_bytes(2) + bytes(4)

def create_remove_packet(idx) -> bytes:
    return mintd_magic + Commands.REMOVE_TORRENT.to_bytes(2) + int(idx).to_bytes(4, signed=False)

def create_invalid_packet() -> bytes:
    tmp = 0xFFFF
    return mintd_magic + tmp.to_bytes(2) + bytes(4)

def mintd_invalid(args):
    packet = create_invalid_packet()
    response = send_packet(packet)
    print(response.decode('utf-8'))


def mintd_status(args):
    packet = create_status_packet()
    response = send_packet(packet)
    print(response.decode('utf-8'))

def create_add_packet(magnet_url) -> bytes:
    return mintd_magic + Commands.ADD.to_bytes(2) + bytes(4) + magnet_url.encode('utf-8')

def mintd_add(args):
    packet = create_add_packet(args.magnet_url)
    response = send_packet(packet)
    print(response.decode('utf-8'))

def mintd_remove(args):
    packet = create_remove_packet(args.num)
    send_packet(packet)

def mintd_stats(args):
    packet = create_query_stats_packet()
    response = send_packet(packet)
    print(response.decode('utf-8'))

def mintd_pause(args):
    idx = args.num
    if not idx.isdigit():
        if idx == 'all':
            packet = create_pause_all_packet()
            send_packet(packet)
        else:
            print(f"input {idx} is invalid")
    else:
        packet = create_pause_packet(idx)
        send_packet(packet)

def mintd_resume(args):
    idx = args.num
    if not idx.isdigit():
        if idx == 'all':
            packet = create_resume_all_packet()
            send_packet(packet)
        else:
            print(f"input {idx} is invalid")
    else:
        packet = create_resume_packet(idx)
        send_packet(packet)

def create_add_parser(subparser):
    add_parser = subparser.add_parser('add')
    add_parser.add_argument('magnet_url')
    add_parser.set_defaults(func=mintd_add)

def create_remove_parser(subparser):
    remove_parser = subparser.add_parser('remove')
    remove_parser.add_argument('num')
    remove_parser.set_defaults(func=mintd_remove)

def create_status_parser(subparser):
    status_parser = subparser.add_parser('status')
    status_parser.set_defaults(func=mintd_status)

def create_pause_parser(subparser):
    pause_parser = subparser.add_parser('pause')
    pause_parser.add_argument('num')
    pause_parser.set_defaults(func=mintd_pause)

def create_resume_parser(subparser):
    pause_parser = subparser.add_parser('resume', aliases=["unpause"])
    pause_parser.add_argument('num')
    pause_parser.set_defaults(func=mintd_resume)

def create_stats_parser(subparser):
    pause_parser = subparser.add_parser('stats') 
    pause_parser.set_defaults(func=mintd_stats)

def create_invalid_parser(subparser):
    pause_parser = subparser.add_parser('invalid')
    pause_parser.set_defaults(func=mintd_invalid)


def parse_cmd():
    parser = argparse.ArgumentParser(description='tiny client for mintd')
    subparser = parser.add_subparsers(dest='cmd', required=True)
    create_add_parser(subparser)
    create_status_parser(subparser)
    create_pause_parser(subparser)
    create_resume_parser(subparser)
    create_remove_parser(subparser)
    create_stats_parser(subparser)
    create_invalid_parser(subparser)
    args = parser.parse_args()
    return args

def main():
    args = parse_cmd()
    args.func(args)

if __name__ == "__main__":
    main()
