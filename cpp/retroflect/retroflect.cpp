// retroflect.cpp : Defines the entry point for the console application.
//

/*
Copyright (C) 2018  twisteroid ambassador

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
Reflect network traffic targeted at specified IP addresses.

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
*/

#include "stdafx.h"

#include "WinDivertWrapper.h"


const INT16 PRIORITY = 823;  // arbitrarily chosen low-ish priority value
const size_t MAXBUFF = 0xFFFF;

/*
Convert text in char array to a port number, i.e. integer between 0 and 65535.
Ensure number is in range, and also ensure no trailing non-number characters.
Meant for converting port numbers specified in argv[].
*/
UINT16 charArrayToPort(char* arg)
{
	size_t size;
	std::string arg_string(arg);
	auto num = std::stoi(arg_string, &size);
	if (size == arg_string.size()) {
		if (num <= static_cast<decltype(num)>(UINT16_MAX)) {
			return num;
		}
		else {     
			throw std::out_of_range("Port number out of range");
		}
	}
	else {
		throw std::invalid_argument("Did not consume entire string");
	}
}


/*
The main reflection logic.
Should be possible to run parallel instances in threads.
*/
void reflect(WinDivertWrapper& divert) {
	char packet_buf[MAXBUFF];
	UINT packet_len;
	WINDIVERT_ADDRESS addr;
	PWINDIVERT_IPHDR ipv4_header;
	PWINDIVERT_IPV6HDR ipv6_header;
	UINT32 tmp_addr[4];

	while (true) {
		divert.Recv(packet_buf, sizeof(packet_buf), &addr, &packet_len);
		if (addr.Direction == WINDIVERT_DIRECTION_OUTBOUND) {
			if (WinDivertHelperParsePacket(
				packet_buf, packet_len, &ipv4_header, nullptr,
				nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
				)) {
				tmp_addr[0] = ipv4_header->DstAddr;
				ipv4_header->DstAddr = ipv4_header->SrcAddr;
				ipv4_header->SrcAddr = tmp_addr[0];
			}
			else if (WinDivertHelperParsePacket(
				packet_buf, packet_len, nullptr, &ipv6_header,
				nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
				)) {
				for (int i = 0; i < 4; ++i) {
					tmp_addr[i] = ipv6_header->DstAddr[i];
					ipv6_header->DstAddr[i] = ipv6_header->SrcAddr[i];
					ipv6_header->SrcAddr[i] = tmp_addr[i];
				}
			}
			else {
				throw std::runtime_error("Error parsing IPv4 and IPv6 header");
			}
			addr.Direction = WINDIVERT_DIRECTION_INBOUND;
			WinDivertHelperCalcChecksums(packet_buf, packet_len, &addr, 0);
			divert.Send(packet_buf, packet_len, &addr, nullptr);
		}
		else {
			// inbound packet matched by filter are dropped
		}
	}
}

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		std::cout << "Usage: " << argv[0] << " <reflect_address_or_shield_port> ..." << std::endl;
		return EXIT_SUCCESS;
	}
	try
	{
		std::vector<char*> reflectIPv6Addrs, reflectIPv4Addrs;
		std::vector<UINT16> shieldPorts;
		for (int i = 1; i < argc; i++) {
			UINT32 addrTemp[4];
			if (WinDivertHelperParseIPv4Address(argv[i], addrTemp)) {
				reflectIPv4Addrs.push_back(argv[i]);
			}
			else if (WinDivertHelperParseIPv6Address(argv[i], addrTemp)) {
				reflectIPv6Addrs.push_back(argv[i]);
			}
			else {
				try {
					auto port = charArrayToPort(argv[i]);
					shieldPorts.push_back(port);
				}
				catch (const std::invalid_argument& e) {
					std::cerr << "Invalid IP address or port number: " << argv[i] << std::endl;
					std::cerr << e.what() << std::endl;
					return EXIT_FAILURE;
				}
				catch (const std::out_of_range& e) {
					std::cerr << "Invalid IP address or port number: " << argv[i] << std::endl;
					std::cerr << e.what() << std::endl;
					return EXIT_FAILURE;
				}
			}
		}
		if (reflectIPv4Addrs.empty() && reflectIPv6Addrs.empty()) {
			std::cerr << "No reflect address specified" << std::endl;
			return EXIT_FAILURE;
		}
		std::cout << "Reflect IP addresses:" << std::endl;
		for (auto addr : reflectIPv4Addrs) {
			std::cout << "  " << addr << std::endl;
		}
		for (auto addr : reflectIPv6Addrs) {
			std::cout << "  " << addr << std::endl;
		}
		std::cout << "Shield Ports:" << std::endl;
		if (shieldPorts.empty()) {
			std::cout << "  None" << std::endl;
		}
		else {
			for (auto port : shieldPorts) {
				std::cout << "  " << port << std::endl;
			}
		}
		std::string filter("!loopback and (tcp or udp) and ((outbound and (");
		bool first = true;
		for (auto addr : reflectIPv4Addrs) {
			if (first) {
				first = false;
			}
			else {
				filter += " or ";
			}
			filter += "ip.DstAddr == ";
			filter += addr;
		}
		for (auto addr : reflectIPv6Addrs) {
			if (first) {
				first = false;
			}
			else {
				filter += " or ";
			}
			filter += "ipv6.DstAddr == ";
			filter += addr;
		}
		filter += "))";

		if (!shieldPorts.empty()) {
			first = true;
			filter += " or (inbound and (";
			for (auto port : shieldPorts) {
				if (first) {
					first = false;
				}
				else {
					filter += " or ";
				}
				filter += "tcp.DstPort == ";
				filter += std::to_string(port);
				filter += " or udp.DstPort == ";
				filter += std::to_string(port);
			}
			filter += "))";
		}

		filter += ")";

		WinDivertWrapper divert(filter, WINDIVERT_LAYER_NETWORK, PRIORITY, 0);
		std::cout << "Reflection in progress..." << std::endl;
		reflect(divert);
		return EXIT_SUCCESS;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Unhandled exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}

