#ifndef dbg_router_h
#define dbg_router_h

#include "cmu-trace.h"
#include "priqueue.h"
#include <classifier/classifier-port.h>
#include "agent.h"

class DBGRouterAgent: public Agent {
public:
	DBGRouterAgent(nsaddr_t taddr);
	virtual ~DBGRouterAgent();
	
	void recv(Packet *p, Handler *h);
	int command(int argc, const char *const* argv);
	
protected:
	PortClassifier *dmux_;
	TclObject *uptarget_;
	Trace *logtarget_;
	nsaddr_t addr_;
};

#endif // dbg_router_h
