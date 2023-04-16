#pragma once

class CNetServer;

class SuperRPCStub {
private:
	friend UINT WINAPI Thread_Worker(PVOID param);
	friend UINT WINAPI Thread_Contents(PVOID param);
	friend class CNetServer;
	friend class NetContents;
	virtual bool __RPC__DispatchRPC(NID nid, SerialBuffer* buf) = 0;
	virtual void __RPC__Init() = 0;
protected:
	CNetServer* __RPC__sv = nullptr;
};

class SuperRPCProxy {
public:
	CNetServer* __RPC__sv = nullptr;
};