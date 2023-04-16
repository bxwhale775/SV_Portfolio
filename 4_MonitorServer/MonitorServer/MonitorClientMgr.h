#pragma once
#include <time.h>
#include "NetSV-include/CNetServer.h"
#include "MonitorProtocol.h"

class MonitorClientMgr {
private:
	CNetServer* _mcli = nullptr;
	int _serverno = -1;
public:
	~MonitorClientMgr() {
		if (_mcli != nullptr) {
			this->Disconnect();
			delete _mcli; 
		}
		return;
	}

	bool Init(const WCHAR* ip, u_short port, int serverno) {
		if (_mcli == nullptr) { _mcli = new CNetServer(); }
		CNetServer_Properties mtprops;
		mtprops.CocurrentThreadNum = 1;
		mtprops.WorkerThreadNum = 1;
		mtprops.Client_RunAsClient = true;
		mtprops.Client_IP = ip;
		mtprops.Client_Port = port;
		mtprops.disableEncrypt = true;
		_serverno = serverno;
		return _mcli->Init(mtprops, nullptr);
	}
	bool Start() {
		if (_mcli == nullptr) { return false; }
		return _mcli->Start();
	}
	bool LoginToMonitor() {
		bool ret = false;
		SerialBuffer* buf = _mcli->AllocPacket();
		st_PACKET_SS_MONITOR_LOGIN pkt;
		pkt.ServerNo = _serverno;
		buf->PutData(&pkt, sizeof(pkt));
		ret = _mcli->SendPacket(NETSV_INVALID_NID, buf);
		_mcli->FreePacket(buf);
		return ret;
	}
	bool SendData(BYTE type, int value) {
		bool ret = false;
		SerialBuffer* buf = _mcli->AllocPacket();
		st_PACKET_SS_MONITOR_DATA_UPDATE pkt;
		pkt.DataType = type;
		pkt.DataValue = value;
		pkt.TimeStamp = (int)time(NULL);
		buf->PutData(&pkt, sizeof(pkt));
		ret = _mcli->SendPacket(NETSV_INVALID_NID, buf);
		_mcli->FreePacket(buf);
		return ret;
	}
	bool Disconnect() {
		return _mcli->Disconnect(NETSV_INVALID_NID);
	}
};

