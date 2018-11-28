#pragma once

#include "stdafx.h"

class WinDivertWrapper
{
	HANDLE m_windivert_handle;
public:
	WinDivertWrapper(std::string const& filter, WINDIVERT_LAYER layer, INT16 priority, UINT64 flags);
	~WinDivertWrapper();

	void Recv(PVOID pPacket, UINT packetLen, PWINDIVERT_ADDRESS pAddr, UINT * recvLen);
	void Send(PVOID pPacket, UINT packetLen, PWINDIVERT_ADDRESS pAddr, UINT * sendLen);
};

