#ifndef dbg_pbagent_h
#define dbg_pbagent_h

#include "ip.h"
#include "agent.h"
#include "dbghistory.h"
#include "dbghdr.h"

#define NOW_TIME (Scheduler::instance().clock())

class PBAgent : public Agent {
public:
	PBAgent(nsaddr_t taddr);
	~PBAgent();
	
	void recv(Packet *p, Handler *h);
	int command(int argc, const char *const* argv);
	
	void send(int len);
protected:
	// protocol parameters
	double prob_;
	double broadcastDelay_;
	double broadcastJitter_;
	
	nsaddr_t addr_;
	History history_;
	static int sendCount_;
	int dataId_;
};

#endif // dbg_pbagent_h