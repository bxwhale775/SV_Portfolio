#pragma once

#pragma pack(push, 1)
struct NetHeader {
	BYTE code = 0;
	WORD len = 0;
	BYTE randkey = 0;
	BYTE chksum = 0;
};
#pragma pack(pop)
