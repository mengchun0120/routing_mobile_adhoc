#include "random.h"
#include "pbagent.h"

int PBAgent::sendCount_ = 0;

//----------------- register class ----------------
class PBAgentClass : public TclClass {
public:
	PBAgentClass(): TclClass("Agent/PB") {}
	TclObject *create(int argc, const char *const * argv)
	{
		assert(argc == 5);
		return new PBAgent(atoi(argv[4]));
	}
} pbagent_class;

//----------------- PBAgent-------------------------
PBAgent::PBAgent(nsaddr_t taddr):
Agent(PT_DBG), history_(), addr_(taddr), dataId_(0)
{
	bind("prob_", &prob_);
	bind("broadcastDelay_", &broadcastDelay_);
	bind("broadcastJitter_", &broadcastJitter_);
}

PBAgent::~PBAgent()
{
}

int PBAgent::command(int argc, const char *const* argv)
{
	Tcl& tcl = Tcl::instance();
	if(argc == 2) {
		if(strcmp(argv[1], "send-count") == 0) {
			tcl.resultf("%d", sendCount_);
			return TCL_OK;
		
		} 
	} else if(argc == 3) {
		if(strcmp(argv[1], "send") == 0) {
			send(atoi(argv[2]));
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "recv-count") == 0) {
			tcl.resultf("%d", history_.count((nsaddr_t)atoi(argv[2])));
			return TCL_OK;
		
		} 
	}
	
	return Agent::command(argc, argv);
}

void PBAgent::recv(Packet *p, Handler *h)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	
	if(history_.insert(dbh->replier_, cmh->uid(), dbh->requestId_, NOW_TIME - cmh->timestamp())) {
		double pr = Random::uniform();
		if(pr < prob_) {
			cmh->direction_ = hdr_cmn::DOWN;
			cmh->num_forwards_ = 0;
			iph->saddr() = addr_;
			++sendCount_;
			Scheduler::instance().schedule(target_, p, 
																broadcastDelay_ + Random::uniform() * broadcastJitter_);
		} else {
			Packet::free(p);
		}
	} else {
		Packet::free(p);
	}
}

void PBAgent::send(int len)
{	
	Packet *p = allocpkt();
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	cmh->size() = len + IP_HDR_LEN + DBG_HDR_LEN;
	cmh->addr_type() = NS_AF_INET;
	cmh->direction_ = hdr_cmn::DOWN;
	cmh->next_hop_ = IP_BROADCAST;
	cmh->num_forwards_ = 0;
	iph->saddr() = addr_;
	iph->sport() = here_.port_;
	iph->daddr() = IP_BROADCAST;
	iph->dport() = here_.port_;
	dbh->replier_ = addr_;
	dbh->requestId_ = dataId_++;
	++sendCount_;
	Scheduler::instance().schedule(target_, p, 
															broadcastDelay_ + Random::uniform() * broadcastJitter_);
}