#ifndef dbg_hdr_h
#define dbg_hdr_h

#include "ip.h"
#include "packet.h"

struct hdr_dbg {
	enum {
		DBG_DATA					= 0x8000,
		DBG_JOIN_REQUEST	= 0x4000,
		DBG_JOIN_REPLY		= 0x2000,
		DBG_JOIN_CONFIRM	= 0x1000,
		DBG_HEARTBEAT			= 0x0800,
		DBG_DATA_REQUEST	= 0x0400,
		DBG_FIND_REQUEST	= 0x0200,
		DBG_FIND_REPLY		= 0x0100,
		DBG_FIND_CONFIRM	= 0x0080,
		DBG_GATEWAY 			= 0x0040,
		DBG_GATEWAY_LEAVE = 0x0020,
		DBG_TEST					= 0x0010
	};
	nsaddr_t groupId_;
	int requestId_;
	nsaddr_t requestor_;
	nsaddr_t replier_;
	nsaddr_t nexthop_;
	int flag_;
	int findCount_;
	int neighborCount_;
	static int offset_;
	static hdr_dbg *access(Packet *p) { return (hdr_dbg*)p->access(offset_); }
};

#define DBG_HDR_LEN			sizeof(hdr_dbg)

#endif // dbg_hdr_h
