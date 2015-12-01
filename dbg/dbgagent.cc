#include "dbghdr.h"
#include "gwmon.h"
#include "dbgagent.h"
#include "random.h"

int hdr_dbg::offset_;
int DBGAgent::sendCount_ = 0;

//------------- DBGHeaderClass -------
class DBGHeaderClass : public PacketHeaderClass {
public:
	DBGHeaderClass() : PacketHeaderClass("PacketHeader/DBG", sizeof(hdr_dbg)) 
	{
		bind_offset(&hdr_dbg::offset_);	
	}
} hdr_dbg_class;

//------------- DBGAgentClass -------
class DBGAgentClass : public TclClass {
public:
	DBGAgentClass(): TclClass("Agent/DBGAgent") {}
	TclObject *create(int argc, const char *const * argv)
	{
		assert(argc == 5);
		return new DBGAgent(atoi(argv[4]));
	}
} dbgagent_class;

//------------- DBGAgent ------------
DBGAgent::DBGAgent(nsaddr_t taddr):
Agent(PT_DBG), groups_(this)
{
	bind("initBroadcastProb_", &initBroadcastProb_);
	bind("dropBroadcastProb_", &dropBroadcastProb_);
	bind("broadcastWait_", &broadcastWait_);
	bind("broadcastWaitDelay_", &broadcastWaitDelay_);
	bind("broadcastDelay_", &broadcastDelay_);
	bind("broadcastJitter_", &broadcastJitter_);
	bind("broadcastListSize_", &broadcastListSize_);
	bind("requestWaitShort_", &requestWaitShort_);
	bind("requestWaitLong_", &requestWaitLong_);
	bind("heartbeatDelay_", &heartbeatDelay_);
	bind("checkDelay_", &checkDelay_);
	bind("leaveThreshold_", &leaveThreshold_);
	bind("findWaitDelay_", &findWaitDelay_);
	bind("initServeTime_", &initServeTime_);
	bind("serveIncrement_", &serveIncrement_);
	bind("waitConfirmDelay_", &waitConfirmDelay_);
	bind("confirmDelay_", &confirmDelay_);
	addr_ = taddr;
}

DBGAgent::~DBGAgent()
{
}

int DBGAgent::command(int argc, const char *const* argv)
{
	Tcl& tcl = Tcl::instance();
	if(argc == 2) {
		if(strcmp(argv[1], "id") == 0) {
			tcl.resultf("%d", addr_);
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "send-count") == 0) {
			tcl.resultf("%d", sendCount_);
			return TCL_OK;
		}
				
	} else if(argc == 3) {
		if(strcmp(argv[1], "addr") == 0) {
			addr_ = (nsaddr_t)atoi(argv[2]);
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "declare-group") == 0) {
			declare((nsaddr_t)atoi(argv[2]));
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "join-group") == 0) {
			join((nsaddr_t)atoi(argv[2]));
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "recv-join-count") == 0) {
			tcl.resultf("%d", recvJoinCount((nsaddr_t)atoi(argv[2])));
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "is-gateway") == 0) {
			tcl.resultf("%d", isgateway((nsaddr_t)atoi(argv[2])));
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "covered") == 0) {
			tcl.resultf("%d", covered((nsaddr_t)atoi(argv[2])));
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "neighbors") == 0) {
			Group *g = groups_.lookup((nsaddr_t)atoi(argv[2]));
			if(g == 0) {
				tcl.result("");
			} else {
				tcl.result(g->neighbors_.list());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "list-gateway") == 0) {
			tcl.result(GatewayMonitor::instance().list((nsaddr_t)atoi(argv[2])));
			return TCL_OK;
			
		} else if(strcmp(argv[1], "connected") == 0) {
			tcl.resultf("%d", GatewayMonitor::instance().connected((nsaddr_t)atoi(argv[2])));
			return TCL_OK;
			
		} else if(strcmp(argv[1], "is-member") == 0) {
			Group *g = groups_.lookup((nsaddr_t)atoi(argv[2]));
			tcl.resultf("%d", g != 0 ? g->join_ : false);
			return TCL_OK;
		
		} else if(strcmp(argv[1], "gateway-count") == 0) {
			tcl.resultf("%d", GatewayMonitor::instance().count((nsaddr_t)atoi(argv[2])));
			return TCL_OK;
			
		} else if(strcmp(argv[1], "send-test") == 0) {
			sendJoinRequest((nsaddr_t)atoi(argv[2]), 255, true);
			return TCL_OK;
			
		}
		
	} else if(argc == 4) {
		if(strcmp(argv[1], "senddata") == 0) {
			nsaddr_t groupId = (nsaddr_t)atoi(argv[2]);
			Group *g = groups_.lookup(groupId);
			if(g == 0 || g->join_ == false) {
				tcl.result("can not send data");
				return TCL_ERROR;
			}
			sendData(groupId, atoi(argv[3]));
			return TCL_OK;
		
		} else if(strcmp(argv[1], "recv-count") == 0) {
			nsaddr_t groupId = (nsaddr_t)atoi(argv[2]);
			Group *g = groups_.lookup(groupId);
			if(g == 0 || g->join_ == false) {
				tcl.result("can not get count");
				return TCL_ERROR;
			}
			tcl.resultf( "%d", g->history_.count((nsaddr_t)atoi(argv[3])) );
			return TCL_OK;
		
		} else if(strcmp(argv[1], "avg-latency") == 0) {
			nsaddr_t groupId = (nsaddr_t)atoi(argv[2]);
			Group *g = groups_.lookup(groupId);
			if(g == 0 || g->join_ == false) {
				tcl.result("can not get latency");
				return TCL_ERROR;
			}
			tcl.resultf( "%.6f", g->history_.avgLatency((nsaddr_t)atoi(argv[3])) );
			return TCL_OK;
		}
	}
	
	return Agent::command(argc, argv);
}

void DBGAgent::recv(Packet *p, Handler *h)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	
	if(dbh->flag_ & hdr_dbg::DBG_JOIN_REQUEST) {
		handleJoinRequest(p);
		
	} else if(dbh->flag_ & hdr_dbg::DBG_JOIN_REPLY) {
		handleJoinReply(p);
		
	} else if(dbh->flag_ & hdr_dbg::DBG_JOIN_CONFIRM) {
		handleJoinConfirm(p);
		
	} else if(dbh->flag_ & hdr_dbg::DBG_HEARTBEAT) {
		handleHeartbeat(p);
		
	} else if(dbh->flag_ & hdr_dbg::DBG_GATEWAY_LEAVE) {
		handleGatewayLeave(p);
		
	} else if(dbh->flag_ & hdr_dbg::DBG_FIND_REQUEST) {
		handleFindRequest(p);
		
	} else if(dbh->flag_ & hdr_dbg::DBG_FIND_REPLY) {
		handleFindReply(p);
				
	} else if(dbh->flag_ & hdr_dbg::DBG_DATA) {
		handleData(p);
		
	} else {
		Packet::free(p);
	}
}

void DBGAgent::join(nsaddr_t groupId)
{
	Group *g = checkGroup(groupId);
	g->join_ = true;
	// it is already in the neighbor of a dominating member so just quit
	if(g->dflag_ || g->neighbors_.count() > 0) {
		#ifdef DBG_DEBUG
		printf("%.5f %d join-easy gid=%d\n", 
						Scheduler::instance().clock(), addr_, groupId);
		#endif
		return; 
	}

	g->joinNear_ = true;
	sendJoinRequest(groupId, 1);
	g->joinTimer_.resched(requestWaitShort_);
}

Group* DBGAgent::checkGroup(nsaddr_t groupId, bool test)
{
	Group *g = groups_.lookup(groupId);
	if(g == 0) {
		g = groups_.add(groupId, broadcastListSize_, test);
	}
	return g;
}

// declare that this node becomes a group member without sending join request
Group* DBGAgent::declare(nsaddr_t groupId)
{
	Group *g = checkGroup(groupId);
	GatewayMonitor& gm = GatewayMonitor::instance();
	gm.declare(groupId);
	g->join_ = true;
	g->dflag_ = true;
	g->serveTime_ = Scheduler::instance().clock();
	g->serveThreshold_ = initServeTime_;
	//g->permanent_ = true;
	g->hbTimer_.resched(0);
	gm.addGateway(groupId, this);
	return g;
}

void DBGAgent::broadcast(Packet *p, bool waitshort)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	cmh->addr_type() = NS_AF_INET;
	cmh->direction_ = hdr_cmn::DOWN;
	cmh->next_hop_ = IP_BROADCAST;
	cmh->num_forwards_ = 0;
	iph->saddr() = addr_;
	iph->sport() = here_.port_;
	iph->daddr() = IP_BROADCAST;
	iph->dport() = here_.port_;
	
	double delay = (waitshort ? broadcastDelay_ / 3 : broadcastDelay_) + 
									Random::uniform() * broadcastJitter_;
	Scheduler::instance().schedule(target_, p, delay);
}

void DBGAgent::broadcastTimeout(nsaddr_t groupId, int uid)
{
	Group *g = groups_.lookup(groupId);
	g->broadcastList_.timeout(uid);
}

void DBGAgent::joinTimeout(nsaddr_t groupId)
{
	Group *g = groups_.lookup(groupId);
	if(g->joinNear_) {
		sendJoinRequest(groupId, 255);
		g->joinNear_ = false;
		g->joinTimer_.resched(requestWaitLong_);
	} else {
	/*	g->dflag_ = true;
		g->serveTime_ = NOW_TIME;
		g->serveThreshold_ = initServeTime_;
		g->hbTimer_.resched(0);
		GatewayMonitor::instance().addGateway(groupId, this);
	*/
	}
}

void DBGAgent::heartbeatTimeout(nsaddr_t groupId)
{
	Group *g = groups_.lookup(groupId);
	if(g->dflag_ == false) return;
	
	Packet *p = allocpkt();
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
		
	dbh->flag_ = hdr_dbg::DBG_HEARTBEAT;
	dbh->groupId_ = g->groupId_;
	dbh->neighborCount_ = g->neighbors_.count();

	int sz = dbh->neighborCount_ * sizeof(nsaddr_t);
	cmh->size() = IP_HDR_LEN + DBG_HDR_LEN + sz;
	if(dbh->neighborCount_ > 0) {
		PacketData *d = new PacketData(sz);
		g->neighbors_.fill((nsaddr_t *)d->data(), dbh->neighborCount_);
		p->setdata((AppData *)d);
	}
		
	broadcast(p);
	g->hbTimer_.resched(heartbeatDelay_);
}

void DBGAgent::findWaitTimeout(FindRequest *freq)
{
	Group *g = groups_.lookup(freq->groupId());
	GatewayMonitor& gm = GatewayMonitor::instance();
	
	if(g->dflag_ == false) return;
		
	if(g->neighbors_.count() == 0) {
//		double t = g->serveThreshold_ - NOW_TIME + g->serveTime_;
//		if(t < 0) {
			#ifdef DBG_DEBUG
			printf("%.5f %d gateway-abandon gid=%d\n", NOW_TIME, addr_, freq->groupId());
			#endif
			
			g->dflag_ = false;
			g->hbTimer_.force_cancel();
			sendGatewayLeave(freq->groupId());
			gm.removeGateway(freq->groupId(), this);
//		}
		
	} else if(freq->finished() == false) {
		if(freq->subEmpty() == false) {
			#ifdef DBG_DEBUG
			printf("%.5f %d find-subs gid=%d\n", NOW_TIME, addr_, freq->groupId());
			#endif
			FindRequest *f = freq->distractSub(g->requestId_++);
			sendFindRequest(f);
			f->resched(findWaitDelay_);
			g->find_ = true;
		}				
	}
	
	if(g->findTable_.finished()) g->find_ = false;
}

void DBGAgent::checkTimeout(nsaddr_t groupId)
{
	Group *g = groups_.lookup(groupId);
	int ncount;
	
	ncount = g->neighbors_.check();
	if(ncount > 0 && g->dflag_) {
		FindRequest *freq = g->findTable_.add(addr_, g->requestId_++);
			
		if( g->neighbors_.checkFind(freq) > 0 ) {
			sendFindRequest(freq);
			g->find_ = true;
			freq->resched(findWaitDelay_);
		}
	}
	g->neighbors_.flush();
	
	if( (ncount = g->neighbors_.count()) == 0 && g->find_ == false ) {
		if(g->dflag_ && NOW_TIME - g->serveTime_ > g->serveThreshold_) {
			#ifdef DBG_DEBUG
			printf("%.5f %d gateway-abandon gid=%d\n", NOW_TIME, addr_, groupId);
			#endif
				
			g->dflag_ = false;
			g->hbTimer_.force_cancel();
			sendGatewayLeave(groupId);
			GatewayMonitor::instance().removeGateway(groupId, this);
		}
		
		if(g->join_ && g->dflag_ == false && g->joinTimer_.status() == TIMER_IDLE) {
			#ifdef DBG_DEBUG
			printf("%.5f %d rejoin-check gid=%d count=%d\n", NOW_TIME, addr_, groupId, ncount);
			#endif
			g->joinNear_ = false;
			sendJoinRequest(groupId, 8);
			g->joinTimer_.resched(requestWaitLong_);
		}
	}
	
	if(g->join_ && g->dflag_ == false && ncount > 0 && 
		NOW_TIME - g->lastConfirmTime_ > confirmDelay_) {
		Neighbor *n = g->neighbors_.first();
		sendConfirm(0, groupId, n->addr(), n->addr());
	#ifdef DBG_DEBUG
		printf("%.5f %d send-prolong gid=%d daddr=%d\n", NOW_TIME, addr_, groupId, n->addr());
		#endif
		g->lastConfirmTime_ = NOW_TIME;
	}
	
	g->checkTimer_.resched(checkDelay_);
}

void DBGAgent::handleJoinRequest(Packet *p)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	Group *g = checkGroup(dbh->groupId_, dbh->flag_ & hdr_dbg::DBG_TEST);
	
	int idx;
	if((idx = g->broadcastList_.lookup(cmh->uid())) >= 0) {
		g->broadcastList_.onrecv(idx);
		Packet::free(p);
		
	} if(g->requestTable_.lookup(dbh->requestor_, dbh->requestId_) != 0) {
		Packet::free(p);
		
	} else if( g->dflag_ ) {
		Request *r = g->requestTable_.insert(dbh->requestor_, dbh->requestId_);
		r->replied() = true;
		if( g->serveThreshold_ - (NOW_TIME - g->serveTime_)  < waitConfirmDelay_ ) {
			g->serveThreshold_ = (NOW_TIME - g->serveTime_) + waitConfirmDelay_;
		}
		sendJoinReply(dbh->requestId_, dbh->groupId_, dbh->requestor_, iph->saddr());
		Packet::free(p);

	} else {
		if(g->broadcastList_.full() || --iph->ttl() <= 0) {
			Packet::free(p);
			return;			
		}
				
		Request *r = g->requestTable_.insert(dbh->requestor_, dbh->requestId_);
		r->prevhop() = iph->saddr();
		g->broadcastList_.commit(p);
	}
}

void DBGAgent::handleJoinReply(Packet *p)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	Group *g = groups_.lookup(dbh->groupId_);
	
	if(dbh->flag_ & hdr_dbg::DBG_GATEWAY) {
		Neighbor *n = g->neighbors_.add(iph->saddr());
		n->setNeighbors((nsaddr_t *)p->accessdata(), dbh->neighborCount_);
	}
	
	if(g->requestTable_.replied(dbh->requestor_, dbh->requestId_) ) {
		Packet::free(p);
		
	} if(dbh->requestor_ == addr_) {
		g->requestTable_.setReplied(addr_, dbh->requestId_, true);
		#ifdef DBG_DEBUG
		printf("%.5f %d send-confirm uid=%d rid=%d gid=%d rp=%d src=%d\n", 
						NOW_TIME, addr_, cmh->uid(),	dbh->requestId_, dbh->groupId_, dbh->replier_, iph->saddr());
		#endif
		sendConfirm(dbh->requestId_, dbh->groupId_, dbh->replier_, iph->saddr());
		g->lastConfirmTime_ = NOW_TIME;
		g->checkTimer_.resched(checkDelay_ * 2);
		g->joinTimer_.force_cancel();
		
		Packet::free(p);
		
	} else if(iph->daddr() == addr_) {
		#ifdef DBG_DEBUG
		printf("%.5f %d forward-reply uid=%d rid=%d gid=%d rp=%d src=%d\n", 
					NOW_TIME, addr_, cmh->uid(), dbh->requestId_, dbh->groupId_, dbh->replier_, iph->saddr());
		#endif
		
		Request *r = g->requestTable_.lookup(dbh->requestor_, dbh->requestId_);
		r->replied() = true;
		r->nexthop() = iph->saddr();
		
		cmh->direction_ = hdr_cmn::DOWN;
		cmh->num_forwards_ = 0;
		cmh->next_hop_ = r->prevhop();
		cmh->size() = IP_HDR_LEN + DBG_HDR_LEN;
		
		iph->saddr() = addr_;
		iph->daddr() = r->prevhop();
		
		dbh->flag_ = hdr_dbg::DBG_JOIN_REPLY;
		if(g->dflag_) {
			dbh->flag_ |= hdr_dbg::DBG_GATEWAY;
			dbh->neighborCount_ = g->neighbors_.count();
			if(dbh->neighborCount_ > 0) {
				int sz = dbh->neighborCount_ * sizeof(nsaddr_t);
				PacketData *d = new PacketData(sz);
				g->neighbors_.fill((nsaddr_t*)d->data(), dbh->neighborCount_);
				p->setdata((AppData *)d);
			} else {
				p->setdata((AppData *)0);
			}
		} else {
			dbh->neighborCount_ = 0;
			p->setdata((AppData *)0);
		}
		
		target_->recv(p);
	
	} else {
		Packet::free(p);
	}
}

void DBGAgent::handleJoinConfirm(Packet *p)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	GatewayMonitor& gm = GatewayMonitor::instance();
	Group *g = groups_.lookup(dbh->groupId_);
	
	if(dbh->flag_ & hdr_dbg::DBG_GATEWAY) {
		Neighbor *n = g->neighbors_.add(iph->saddr());
		n->setNeighbors((nsaddr_t *)p->accessdata(), dbh->neighborCount_);
	}
	
	if(addr_ == dbh->replier_) {
		#ifdef DBG_DEBUG
		if(dbh->flag_ & hdr_dbg::DBG_GATEWAY) {
			printf("%.5f %d dom-last uid=%d gid=%d req=%d src=%d\n", 
						NOW_TIME, addr_, cmh->uid(), dbh->groupId_, dbh->requestor_, iph->saddr());
		}
		#endif
		if(g->dflag_ && (g->serveThreshold_ - NOW_TIME + g->serveTime_) < serveIncrement_) {
			g->serveThreshold_ = NOW_TIME - g->serveTime_ + serveIncrement_;
			#ifdef DBG_DEBUG
			printf("%.5f %d prolong uid=%d gid=%d thresh=%f serve=%f\n", 
										NOW_TIME, addr_, cmh->uid(), dbh->groupId_, 
										g->serveThreshold_, g->serveTime_);
			#endif
		} else if(dbh->flag_ & hdr_dbg::DBG_GATEWAY == 0) {
			#ifdef DBG_DEBUG
			printf("%.5f %d refuse-prolong uid=%d gid=%d\n", 
										NOW_TIME, addr_, cmh->uid(), dbh->groupId_);
			#endif
		}
		
		Packet::free(p);
	
	} else if(addr_ == iph->daddr()) {
		#ifdef DBG_DEBUG
		printf("%.5f %d dom-forward uid=%d gid=%d req=%d src=%d\n", 
					NOW_TIME, addr_, cmh->uid(), dbh->groupId_, dbh->requestor_, iph->saddr());
		#endif
		Request *r = g->requestTable_.lookup(dbh->requestor_, dbh->requestId_);
		
		if(g->dflag_ == false) {
			g->dflag_ = true;
			g->serveTime_ = NOW_TIME;
			g->serveThreshold_ = initServeTime_;
			g->hbTimer_.resched(0);
			if(gm.lookupGateway(dbh->groupId_, this) == 0) {
				gm.addGateway(dbh->groupId_, this);
			}
		}
		
		iph->saddr() = addr_;
		iph->daddr() = r->nexthop();
		cmh->next_hop_ = r->nexthop();
		cmh->num_forwards_ = 0;
		cmh->direction_ = hdr_cmn::DOWN;
		dbh->flag_ = hdr_dbg::DBG_JOIN_CONFIRM | hdr_dbg::DBG_GATEWAY;
		dbh->neighborCount_ = g->neighbors_.count();
		
		int sz = dbh->neighborCount_ * sizeof(nsaddr_t);
		cmh->size() = IP_HDR_LEN + DBG_HDR_LEN + sz;	
		if(dbh->neighborCount_ > 0) {
			PacketData *d = new PacketData(sz);
			g->neighbors_.fill((nsaddr_t *)d->data(), dbh->neighborCount_);
			p->setdata((AppData *)d);
		} else {
			p->setdata((AppData *)0);
		}
		
		target_->recv(p);	
	} else {
		Packet::free(p);
	}
}

void DBGAgent::handleHeartbeat(Packet *p)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);
	
	Group *g = checkGroup(dbh->groupId_);
	if(g->join_) {
		g->joinTimer_.force_cancel();
	}

	Neighbor *n = g->neighbors_.add(iph->saddr());
	n->setNeighbors((nsaddr_t *)p->accessdata(), dbh->neighborCount_);
	
	if(g->dflag_) {
		ponder(dbh->groupId_);
	}

	Packet::free(p);
}

void DBGAgent::handleGatewayLeave(Packet *p)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	Group *g = checkGroup(dbh->groupId_);
	
	g->neighbors_.remove(dbh->replier_);
	g->neighbors_.purge(dbh->replier_);
	if(g->dflag_) {
		if(g->neighbors_.count() == 0 && g->find_ == false) {
			double t = g->serveThreshold_ - NOW_TIME + g->serveTime_;
			if(t < 0) {
				#ifdef DBG_DEBUG
				printf("%.5f %d gateway-abandon gid=%d\n", NOW_TIME, addr_, dbh->groupId_);
				#endif
				g->dflag_ = false;
				g->hbTimer_.force_cancel();
				GatewayMonitor::instance().removeGateway(dbh->groupId_, this);
			}
		} else {
			ponder(dbh->groupId_);
		}
	}
	
	if(g->join_) {
		if(g->neighbors_.count() == 0 && g->joinTimer_.status() == TIMER_IDLE && g->find_ == false) {
			#ifdef DBG_DEBUG
			printf("%.5f %d rejoin gid=%d\n", NOW_TIME, addr_, dbh->groupId_);
			#endif
			g->joinNear_ = false;
			sendJoinRequest(dbh->groupId_, 8);
			g->joinTimer_.resched(requestWaitLong_);		
		}
	}
			
	if(--(iph->ttl()) > 0) {
		broadcast(p, true);
	} else {
		Packet::free(p);
	}
}

void DBGAgent::handleFindRequest(Packet *p)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	Group *g = checkGroup(dbh->groupId_);
	nsaddr_t *list = (nsaddr_t *)(p->accessdata());
	int idx;

	if(dbh->flag_ & hdr_dbg::DBG_GATEWAY) {
		Neighbor *n = g->neighbors_.add(iph->saddr());
		n->setNeighbors(list, dbh->neighborCount_);
	}
	
	if((idx = g->broadcastList_.lookup(cmh->uid())) >= 0) {
		g->broadcastList_.onrecv(idx);
		Packet::free(p);	
	
	} else if(g->findTable_.lookup(dbh->requestor_, dbh->requestId_)) {
		Packet::free(p);
	
	} else {
		bool reply;
		FindRequest *f = g->findTable_.addMarked(dbh->requestor_, dbh->requestId_, 
																	list + dbh->neighborCount_, dbh->findCount_, 
																	reply, iph->saddr());
		
		if(reply) {
			sendFindReply(dbh->groupId_, dbh->requestor_, dbh->requestId_, f->prevhop());
			g->checkTimer_.resched(checkDelay_ * 2);
		}
		
		if( --(iph->ttl()) > 0 && (!reply || dbh->findCount_ > 1) ) {
			forwardFindRequest(p, f);
		} else {
			Packet::free(p);
		}
	}
}

void DBGAgent::handleFindReply(Packet *p)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	Group *g = groups_.lookup(dbh->groupId_);
	GatewayMonitor& gm = GatewayMonitor::instance();
	
	if(dbh->flag_ & hdr_dbg::DBG_GATEWAY) {
		Neighbor *n = g->neighbors_.add(iph->saddr());
		n->setNeighbors((nsaddr_t *)p->accessdata(), dbh->neighborCount_);
	}
	
	if(dbh->requestor_ == addr_) {
		FindRequest *freq = g->findTable_.lookup(dbh->requestor_, dbh->requestId_);
		FindEntry *fe = freq->lookupFind(dbh->replier_);
		
		fe->find() = true;
		if(freq->finished()) freq->force_cancel();
		if( g->findTable_.finished() ) g->find_ = false;
		
		#ifdef DBG_DEBUG
		printf("%.5f %d find-reply-last uid=%d gid=%d req=%d rid=%d src=%d replier=%d find=%d\n", 
						NOW_TIME, addr_, cmh->uid(),dbh->groupId_, dbh->requestor_, 
						dbh->requestId_, iph->saddr(), dbh->replier_, g->find_);
		#endif

		Packet::free(p);

	} else if(iph->daddr() == addr_) {	
		if(g->dflag_ == false) {
			g->dflag_ = true;
			g->serveTime_ = NOW_TIME;
			g->serveThreshold_ = initServeTime_;
			g->hbTimer_.resched(0);
			if(gm.lookupGateway(dbh->groupId_, this) == 0) {
				gm.addGateway(dbh->groupId_, this);
			}
		}

		#ifdef DBG_DEBUG
		printf("%.5f %d forward-find-reply uid=%d gid=%d req=%d rid=%d src=%d replier=%d\n", 
						NOW_TIME, addr_, cmh->uid(), dbh->groupId_, dbh->requestor_, 
						dbh->requestId_, iph->saddr(), dbh->replier_);
		#endif
		
		FindRequest *freq = g->findTable_.lookup(dbh->requestor_, dbh->requestId_);

		cmh->num_forwards_ = 0;
		cmh->next_hop_ = freq->prevhop();
		cmh->direction_ = hdr_cmn::DOWN;

		iph->saddr() = addr_;
		iph->daddr() = freq->prevhop();		

		dbh->flag_ = hdr_dbg::DBG_FIND_REPLY | hdr_dbg::DBG_GATEWAY;
		dbh->neighborCount_ = g->neighbors_.count();
		
		int sz = dbh->neighborCount_ * sizeof(nsaddr_t);
		cmh->size() = IP_HDR_LEN + DBG_HDR_LEN + sz;	
		if(dbh->neighborCount_ > 0) {
			PacketData *d = new PacketData(sz);
			g->neighbors_.fill((nsaddr_t *)d->data(), dbh->neighborCount_);
			p->setdata((AppData *)d);
		} else {
			p->setdata((AppData *)0);
		}
		
		target_->recv(p);
	
	} else {
		Packet::free(p);
	}
}

void DBGAgent::handleData(Packet *p)
{
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	Group *g = groups_.lookup(dbh->groupId_);
	
	if(dbh->flag_ & hdr_dbg::DBG_GATEWAY) {
		Neighbor *n = g->neighbors_.add(iph->saddr());
		n->setNeighbors((nsaddr_t *)p->accessdata(), dbh->neighborCount_);
	}
	
	if(g->join_ || g->dflag_) {
		bool rec = g->history_.insert(dbh->replier_, cmh->uid(), 
																	dbh->requestId_, NOW_TIME - cmh->timestamp());
		if(rec && g->dflag_) {
			dbh->flag_ = hdr_dbg::DBG_DATA | hdr_dbg::DBG_GATEWAY;
			dbh->neighborCount_ = g->neighbors_.count();
			
			int sz = dbh->neighborCount_ * sizeof(nsaddr_t);
			cmh->size() = IP_HDR_LEN + DBG_HDR_LEN + dbh->findCount_ + sz;
			if(dbh->neighborCount_ > 0) {
				PacketData *d = new PacketData(sz);
				g->neighbors_.fill((nsaddr_t *)d->data(), dbh->neighborCount_);
				p->setdata((AppData *)d);
			} else {
				p->setdata((AppData *)0);
			}
			
			broadcast(p);
		} else {
			Packet::free(p);
		}
		
	} else {
		Packet::free(p);
	}
}

void DBGAgent::sendData(nsaddr_t groupId, int size)
{
	Group *g = groups_.lookup(groupId);
	Packet *p = allocpkt();
	hdr_cmn *cmh = hdr_cmn::access(p);
	
	cmh->size() = IP_HDR_LEN + DBG_HDR_LEN + size;
	
	hdr_dbg *dbh = hdr_dbg::access(p);
	dbh->flag_ = hdr_dbg::DBG_DATA;
	if(g->dflag_) dbh->flag_ |= hdr_dbg::DBG_GATEWAY;
	dbh->replier_ = addr_;					// source of the data
	dbh->requestId_ = g->dataId_++;	// id of the data
	dbh->findCount_ = size;					// size of the data
	if(g->dflag_) {
		dbh->neighborCount_ = g->neighbors_.count();
		if(dbh->neighborCount_ > 0) {
			int sz = dbh->neighborCount_ * sizeof(nsaddr_t);
			cmh->size() += sz;
			PacketData *d = new PacketData(sz);
			g->neighbors_.fill((nsaddr_t *)d->data(), dbh->neighborCount_);
			p->setdata((AppData *)d);
		}
	}
	
	g->history_.insert(addr_, cmh->uid(), dbh->requestId_, 0);
	
	broadcast(p);
}

void DBGAgent::sendJoinRequest(nsaddr_t groupId, int ttl, bool test)
{
	Group *g = checkGroup(groupId, test);
	Packet *p = allocpkt();
	hdr_ip *iph = hdr_ip::access(p);
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	
	cmh->size() = IP_HDR_LEN + DBG_HDR_LEN;
	iph->ttl() = ttl;
	
	dbh->flag_ = hdr_dbg::DBG_JOIN_REQUEST;
	if(test) dbh->flag_ |= hdr_dbg::DBG_TEST;
	dbh->requestId_ = g->requestId_++;
	dbh->groupId_ = g->groupId_;
	dbh->requestor_ = addr_;

	g->requestTable_.insert(addr_, dbh->requestId_);
	
	#ifdef DBG_DEBUG
	if(!test) {
		if(ttl == 1) {
			printf("%.5f %d send-join-near uid=%d rid=%d gid=%d\n", 
							NOW_TIME, addr_, cmh->uid(), dbh->requestId_, dbh->groupId_);
		} else {
			printf("%.5f %d send-join-far uid=%d rid=%d gid=%d\n", 
							NOW_TIME, addr_, cmh->uid(), dbh->requestId_, dbh->groupId_);
		}
	}
	#endif
	
	if(test) ++sendCount_;
	broadcast(p);
}

void DBGAgent::sendJoinReply(int requestId, nsaddr_t groupId, 
						nsaddr_t requestor, nsaddr_t nexthop)
{
	Packet *p = allocpkt();
	Group *g = groups_.lookup(groupId);
	
	hdr_cmn *cmh = hdr_cmn::access(p);
	cmh->addr_type() = NS_AF_INET;
	cmh->size() = IP_HDR_LEN + DBG_HDR_LEN;
	cmh->direction_ = hdr_cmn::DOWN;
	cmh->num_forwards_ = 0;
	cmh->next_hop_ = nexthop;
	
	hdr_ip *iph = hdr_ip::access(p);
	iph->saddr() = addr_;
	iph->sport() = here_.port_;
	iph->daddr() = nexthop;
	iph->dport() = here_.port_;
	
	hdr_dbg *dbh = hdr_dbg::access(p);
	dbh->flag_ = hdr_dbg::DBG_JOIN_REPLY | hdr_dbg::DBG_GATEWAY;
	dbh->groupId_ = groupId;
	dbh->requestId_ = requestId;
	dbh->replier_ = addr_;
	dbh->requestor_ = requestor;
	dbh->neighborCount_ = g->neighbors_.count();
	
	if(dbh->neighborCount_ > 0) {
		int sz = dbh->neighborCount_ * sizeof(nsaddr_t);
		cmh->size() += sz;
		PacketData *d = new PacketData(sz);
		g->neighbors_.fill((nsaddr_t *)d->data(), dbh->neighborCount_);
		p->setdata((AppData *)d);
	}
	
	#ifdef DBG_DEBUG
	printf("%.5f %d send-join-reply uid=%d rid=%d gid=%d req=%d src=%d\n", 
					NOW_TIME, addr_, cmh->uid(), dbh->requestId_, dbh->groupId_, dbh->requestor_, nexthop);
	#endif
	
	target_->recv(p);
}

void DBGAgent::sendConfirm(int requestId, nsaddr_t groupId, 
						nsaddr_t replier, nsaddr_t nexthop)
{
	Packet *p = allocpkt();
	
	hdr_cmn *cmh = hdr_cmn::access(p);
	cmh->size() = IP_HDR_LEN + DBG_HDR_LEN;
	cmh->direction_ = hdr_cmn::DOWN;
	cmh->num_forwards_ = 0;
	cmh->next_hop_ = nexthop;
	cmh->addr_type() = NS_AF_INET;
	
	hdr_ip *iph = hdr_ip::access(p);
	iph->daddr() = nexthop;
	iph->dport() = here_.port_;
		
	hdr_dbg *dbh = hdr_dbg::access(p);
	dbh->flag_ = hdr_dbg::DBG_JOIN_CONFIRM;
	dbh->groupId_ = groupId;
	dbh->requestId_ = requestId;
	dbh->requestor_ = addr_;
	dbh->replier_ = replier;
	
	target_->recv(p);
}

void DBGAgent::sendGatewayLeave(nsaddr_t groupId)
{
	Packet *p = allocpkt();
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	cmh->size() = IP_HDR_LEN + DBG_HDR_LEN;
	iph->ttl() = 2;
	
	hdr_dbg *dbh = hdr_dbg::access(p);
	dbh->flag_ = hdr_dbg::DBG_GATEWAY_LEAVE;
	dbh->groupId_ = groupId;
	dbh->replier_ = addr_;
	
	#ifdef DBG_DEBUG
	printf("%.5f %d gateway-leave gid=%d uid=%d\n", NOW_TIME, addr_, groupId, cmh->uid());
	#endif
	
	broadcast(p, true);
}

void DBGAgent::sendFindRequest(FindRequest *freq)
{
	Packet *p = allocpkt();
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);	
	Group *g = groups_.lookup(dbh->groupId_);
	
	iph->ttl() = 3;
	
	dbh->flag_ = hdr_dbg::DBG_FIND_REQUEST | hdr_dbg::DBG_GATEWAY;
	dbh->groupId_ = freq->groupId();
	dbh->requestor_ = addr_;
	dbh->requestId_ = freq->requestId();
	dbh->neighborCount_ = g->neighbors_.count();
	dbh->findCount_ = freq->findCount(false);
	
	int sz = (dbh->neighborCount_ + dbh->findCount_) * sizeof(nsaddr_t);
	cmh->size() = IP_HDR_LEN + DBG_HDR_LEN + sz;
	PacketData *d = new PacketData(sz);
	nsaddr_t *list = (nsaddr_t *)d->data();
	
	if(dbh->neighborCount_ > 0) {
		g->neighbors_.fill(list, dbh->neighborCount_);
	}
	freq->fillFind(list + dbh->neighborCount_, dbh->findCount_, false);
	p->setdata((AppData *)d);

	#ifdef DBG_DEBUG
	printf("%.5f %d find-gateway uid=%d gid=%d req=%d src=%d findlist=", NOW_TIME, 
					addr_, cmh->uid(), freq->groupId(), dbh->requestId_, dbh->requestor_);
	list += dbh->neighborCount_;
	for(int i = 0; i < dbh->findCount_; ++i) {
		printf("%d,", list[i]);
	}
	printf("\n");
	#endif
	
	broadcast(p, true);
}

void DBGAgent::sendFindReply(nsaddr_t groupId, nsaddr_t requestor, 
						nsaddr_t requestId, nsaddr_t nexthop)
{
	Group *g = groups_.lookup(groupId);
	Packet *p = allocpkt();
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
		
	cmh->addr_type_ = NS_AF_INET;
	cmh->size() = IP_HDR_LEN + DBG_HDR_LEN;
	cmh->num_forwards_ = 0;
	cmh->next_hop_ = nexthop;
	cmh->direction_ = hdr_cmn::DOWN;
	
	iph->daddr() = nexthop;
	iph->dport() = here_.port_;
	
	dbh->flag_ = hdr_dbg::DBG_FIND_REPLY;
	if(g->dflag_) dbh->flag_ |= hdr_dbg::DBG_GATEWAY;
	dbh->groupId_ = groupId;
	dbh->requestId_ = requestId;
	dbh->requestor_ = requestor;
	dbh->replier_ = addr_;
	dbh->neighborCount_ = g->neighbors_.count();
	
	if(dbh->neighborCount_ > 0) {
		int sz = dbh->neighborCount_ * sizeof(nsaddr_t);
		cmh->size() += sz;
		PacketData *d = new PacketData(sz);
		g->neighbors_.fill((nsaddr_t *)d->data(), dbh->neighborCount_);
		p->setdata((AppData *)d);
	}
	
	#ifdef DBG_DEBUG
	printf("%.5f %d find-reply uid=%d gid=%d req=%d src=%d replier=%d find=%d\n", NOW_TIME, 
					addr_, cmh->uid(), dbh->groupId_, dbh->requestId_, dbh->requestor_, addr_, g->find_);
	#endif
	
	target_->recv(p);
}										

void DBGAgent::forwardFindRequest(Packet *p, FindRequest *freq)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_dbg *dbh = hdr_dbg::access(p);
	Group *g = groups_.lookup(dbh->groupId_);
	
	dbh->flag_ = hdr_dbg::DBG_FIND_REQUEST;
	if(g->dflag_) dbh->flag_ |= hdr_dbg::DBG_GATEWAY;
	dbh->neighborCount_ = g->neighbors_.count();
	dbh->findCount_ = freq->findCount(false);
	
	int sz = (dbh->neighborCount_ + dbh->findCount_) * sizeof(nsaddr_t);
	cmh->size() = IP_HDR_LEN + DBG_HDR_LEN + sz;
	PacketData *d = new PacketData(sz);
	nsaddr_t *list = (nsaddr_t *)d->data();
	
	if(dbh->neighborCount_ > 0) {
		g->neighbors_.fill(list, dbh->neighborCount_);
	}
	freq->fillFind(list + dbh->neighborCount_, dbh->findCount_, false);
	p->setdata((AppData *)d);
	
	if(g->dflag_) {
		#ifdef DBG_DEBUG
		printf("%.5f %d forward-find uid=%d gid=%d req=%d src=%d\n", NOW_TIME, 
						addr_, cmh->uid(), freq->groupId(), freq->requestId(), freq->requestor());
		#endif
		broadcast(p, true);		// gateway broadcasts with higher priority
		
	} else {
		// non-gateway backoff-broadcasts
		if(g->broadcastList_.full() == false) {
			g->broadcastList_.commit(p);
		} else {
			Packet::free(p);
		}
	}
}

// decide if this node remains a gateway or not
// return true if this node has changed to unmarked
bool DBGAgent::ponder(nsaddr_t groupId)
{
	Group *g = groups_.lookup(groupId);
	if(g->dflag_ == false || g->find_) return false;
	if(g->neighbors_.count() == 0 ||g->neighbors_.ponderClassOne() || g->neighbors_.ponderClassTwo()) {
		double t = g->serveThreshold_ - NOW_TIME + g->serveTime_;
		if(t <= 0) {
			#ifdef DBG_DEBUG
			printf("%.5f %d ponder-leave gid=%d\n", NOW_TIME, addr_, groupId);
			#endif
			
			g->dflag_ = false;
			g->hbTimer_.force_cancel();
			sendGatewayLeave(groupId);
			GatewayMonitor::instance().removeGateway(groupId, this);
			return true;
		}
	}
	return false;
}

int DBGAgent::recvJoinCount(nsaddr_t groupId)
{
	Group *g = groups_.lookup(groupId);
	if(g == 0) return 0;
	
	return g->requestTable_.count();
}

bool DBGAgent::isgateway(nsaddr_t groupId)
{
	Group *g = groups_.lookup(groupId);
	if(g == 0) return false;
	
	return g->dflag_;
}

bool DBGAgent::covered(nsaddr_t groupId)
{
	Group *g = groups_.lookup(groupId);
	if(g == 0) return false;
	
	return (g->dflag_ || g->neighbors_.count() > 0);
}

NeighborList* DBGAgent::neighbors(nsaddr_t groupId)
{
	Group *g = groups_.lookup(groupId);
	return (g != 0) ? &g->neighbors_ : 0;
}

FindRequestList* DBGAgent::findTable(nsaddr_t groupId)
{
	Group *g = groups_.lookup(groupId);
	return (g != 0) ? &g->findTable_ : 0;
}

bool DBGAgent::gateway(nsaddr_t groupId)
{
	Group *g = groups_.lookup(groupId);
	return (g != 0) ? g->dflag_ : false;
}
