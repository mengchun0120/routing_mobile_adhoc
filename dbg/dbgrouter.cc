#include "dbghdr.h"
#include "dbgrouter.h"

//------------ register class ------------------
class DBGRouterAgentClass : public TclClass {
public:
	DBGRouterAgentClass(): TclClass("Agent/DBGRouter") {}
	TclObject *create(int argc, const char *const * argv)
	{
		assert(argc == 5);
		return new DBGRouterAgent(atoi(argv[4]));
	}
} dbgrouter_class;

//----------- DBGAgent ----------------------
DBGRouterAgent::DBGRouterAgent(nsaddr_t taddr):
Agent(PT_DBGRTR)
{
	addr_ = taddr;
}

DBGRouterAgent::~DBGRouterAgent()
{
}
	
int DBGRouterAgent::command(int argc, const char *const* argv)
{
	Tcl& tcl = Tcl::instance();
	if(argc == 2) {
		if(strcmp(argv[1], "id") == 0) {
			tcl.resultf("%d", addr_);
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "uptarget") == 0) {
			if(uptarget_ != 0) {
				tcl.resultf("%s", uptarget_->name());
			}
			return (TCL_OK);
		} 
				
	} else if(argc == 3) {
		if(strcmp(argv[1], "addr") == 0) {
			addr_ = (nsaddr_t)atoi(argv[2]);
			return (TCL_OK);
		
		} else if(strcmp(argv[1], "log-target") == 0 ||
							strcmp(argv[1], "tracetarget") == 0) {
			logtarget_ = (Trace *)TclObject::lookup(argv[2]);
			if(logtarget_ == 0){
				tcl.resultf("%s cannot find object %s", argv[1], argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "uptarget") == 0) {
			uptarget_ = (TclObject *)TclObject::lookup(argv[2]);
			if(uptarget_ == 0) {
				tcl.resultf("%s: no such object %s", argv[1], argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "port-dmux") == 0) {
			dmux_ = (PortClassifier *)TclObject::lookup(argv[2]);
			if(dmux_ == 0) {
				tcl.resultf("%s: no such object %s", argv[1], argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);

		}
	} 
	
	return Agent::command(argc, argv);
}

void DBGRouterAgent::recv(Packet *p, Handler *h)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	
	if(iph->saddr() == addr_ && cmh->num_forwards_ == 0) {
		Scheduler::instance().schedule(target_, p, 0.0);
	} else if(iph->saddr() == addr_) {
		drop(p, DROP_RTR_ROUTE_LOOP);
	} else if(iph->daddr() == IP_BROADCAST || iph->daddr() == addr_) {
		dmux_->recv(p, (Handler *)0);
	} else {
		Packet::free(p);
	}
}

