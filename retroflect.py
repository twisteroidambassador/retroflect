# Copyright (c) 2018 twisteroid ambassador. Licensed under GNU GPL v3.

"""Reflect network traffic targeted at specified IP addresses.

Any outbound network traffic with destination matching specified
"reflect addresses" are reflected, so they appear as incoming from that address
instead.

Additionally, if any shield ports are specified, incoming network traffic to
these ports are dropped.

The intended use case is: a server running on localhost is bound to
<wildcard address>:<server port>. Any network traffic sent from localhost to
<reflect address>:<server port> is received by the server, making it look like
the server is running on <reflect address>. Additionally, if <server port> is
specified as shield port, the server is protected from other hosts on the
network even though it is bound to the wildcard address.
"""

import logging
import ipaddress
import argparse
import sys
from typing import List, Union

import pydivert


DEFAULT_PRIORITY = 822  # A low-ish priority, chosen randomly

IPAddressType = Union[ipaddress.IPv4Address, ipaddress.IPv6Address]

logger = logging.getLogger(__package__)


def reflect(
        reflect_hosts: List[IPAddressType],
        shield_ports: List[int],
        priority: int,
):
    filter_hosts = ' or '.join(
        f'{"ipv6" if host.version == 6 else "ip"}.DstAddr == {host}'
        for host in reflect_hosts
    )
    filter_str = f'outbound and (tcp or udp) and ({filter_hosts})'
    if shield_ports:
        filter_ports = ' or '.join(
            f'tcp.DstPort == {port} or udp.DstPort == {port}' for port in shield_ports)
        filter_str = f'({filter_str}) or ' \
                     f'(inbound and ({filter_ports}))'

    with pydivert.WinDivert(filter_str, priority=priority) as wd:
        for packet in wd:
            logger.debug('Received packet:\n%r', packet)
            if packet.is_outbound:
                (packet.src_addr, packet.dst_addr) = \
                    (packet.dst_addr, packet.src_addr)
                packet.direction = pydivert.Direction.INBOUND
                logger.debug('Reflecting packet:\n%r', packet)
                wd.send(packet)
            else:
                logger.debug('Dropping packet')


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        'reflect_address', nargs='+',
        type=ipaddress.ip_address,
        help='IP address(es) to reflect from.')
    parser.add_argument(
        '-s', '--shield', action='append',
        help='Shield port. Can be specified multiple times.')
    parser.add_argument(
        '-v', '--verbose', action='store_true', help='Enable debug logging.')
    parser.add_argument(
        '-p', '--priority', type=int, default=DEFAULT_PRIORITY,
        help='''WinDivert handle priority level. Must be between -1000 
        (highest priority) and 1000 (lowest priority) inclusive. There's 
        usually no need to worry about this unless several programs using 
        WinDivert is run simultaneously. In that case consult WinDivert 
        documentation.''')

    args = parser.parse_args()

    loglevel = logging.DEBUG if args.verbose else logging.WARNING
    logging.basicConfig(level=loglevel)

    if not -1000 <= args.priority <= 1000:
        sys.exit('Priority level must be between -1000 and 1000 inclusive')

    try:
        reflect(args.reflect_address, args.shield, args.priority)
    except PermissionError as e:
        sys.exit(f'Caught PermissionError: are you running this program '
                 f'with Administrator privileges?\n{e!r}')


if __name__ == '__main__':
    main()
