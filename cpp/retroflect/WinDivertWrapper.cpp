#include "stdafx.h"
#include "WinDivertWrapper.h"


WinDivertWrapper::WinDivertWrapper(std::string const& filter, WINDIVERT_LAYER layer, INT16 priority, UINT64 flags)
{
	m_windivert_handle = WinDivertOpen(filter.c_str(), layer, priority, flags);
	if (m_windivert_handle == INVALID_HANDLE_VALUE) {
		auto errcode = GetLastError();
		throw std::runtime_error("Error opening WinDivert handle, error code: " + std::to_string(errcode));
	}
}


WinDivertWrapper::~WinDivertWrapper()
{
	if (!WinDivertClose(m_windivert_handle)) {
		auto errcode = GetLastError();
		std::cerr << "Error closing WinDivert handle, error code: " << errcode << std::endl;
	}
}

void WinDivertWrapper::Recv(PVOID pPacket, UINT packetLen, PWINDIVERT_ADDRESS pAddr, UINT * recvLen)
{
	if (!WinDivertRecv(m_windivert_handle, pPacket, packetLen, pAddr, recvLen)) {
		auto errcode = GetLastError();
		throw std::runtime_error("Error receiving packet, error code: " + std::to_string(errcode));
	}
}

void WinDivertWrapper::Send(PVOID pPacket, UINT packetLen, PWINDIVERT_ADDRESS pAddr, UINT * sendLen)
{
	if (!WinDivertSend(m_windivert_handle, pPacket, packetLen, pAddr, sendLen)) {
		auto errcode = GetLastError();
		if (errcode == ERROR_HOST_UNREACHABLE) {
			/* This error occurs when an impostor packet (with pAddr->Impostor set to 1) 
			is injected and the ip.TTL or ipv6.HopLimit field goes to zero. This is a 
			defense of "last resort" against infinite loops caused by impostor packets. */
			return;
		}
		throw std::runtime_error("Error sending packet, error code: " + std::to_string(errcode));
	}
}
