#pragma once

using NID = unsigned __int64;		//NetSession ID
using CID = unsigned long;			//Contents ID

enum class en_NETSV_MTR_TYPE {
	MTR_NULL,
	MTR_INT_POOL_CAP_NSESS,
	MTR_INT_POOL_USE_NSESS,
	MTR_INT_POOL_CAP_PKT,
	MTR_INT_POOL_USE_PKT,
	MTR_ULL_NET_CNT_ACCEPT,
	MTR_ULL_NET_CNT_SEND,
	MTR_ULL_NET_CNT_RECV,
};