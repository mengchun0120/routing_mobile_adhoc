#include "dbgbclist.h"
#include "dbgagent.h"
#include "random.h"

//----------- BroadcastWaitTimer -----------
void BroadcastWaitTimer::putPacket(Packet *p)
{
	if(idle()) pkt_ = p;
}

void BroadcastWaitTimer::start(double prob)
{
	if(!initialized()) return;
	prob_ = prob;
	int k = (int)(a_->broadcastWait() * Random::uniform());
	resched(k * a_->broadcastDelay());
}

void BroadcastWaitTimer::free()
{
	pkt_ = 0;
	force_cancel();
}

void BroadcastWaitTimer::drop()
{
	Packet::free(pkt_);
	free();
}
	
void BroadcastWaitTimer::expire(Event *e)
{
	hdr_cmn *cmh = hdr_cmn::access(pkt_);
	hdr_dbg *dbh = hdr_dbg::access(pkt_);
	
	a_->broadcastTimeout(dbh->groupId_, cmh->uid());
}

//------------- BroadcastList --------------
BroadcastList::BroadcastList(int size, DBGAgent *a):
size_(size), count_(0), a_(a)
{
	list_ = new BroadcastWaitTimer[size_];
	for(int i = 0; i < size_; ++i) {
		list_[i].setAgent(a);
	}
}

BroadcastList::~BroadcastList()
{
	for(int i = 0; i < size_; ++i) {
		list_[i].force_cancel();
	}
	delete[] list_;
}

int BroadcastList::lookup(int uid)
{
	hdr_cmn *cmh;
	int i;
	for(i = 0; i < size_; ++i) {
		if(list_[i].idle() == false) {
			cmh = hdr_cmn::access(list_[i].getPacket());
			if(cmh->uid() == uid) {
				return i;
			}
		}
	}
	return -1;
}

int BroadcastList::lookupRequest(nsaddr_t saddr, int requestId)
{
	hdr_dbg *dbh;
	int i;
	for(i = 0; i < size_; ++i) {
		if(list_[i].idle() == false) {
			dbh = hdr_dbg::access(list_[i].getPacket());
			if((dbh->flag_ & hdr_dbg::DBG_JOIN_REQUEST) && 
				dbh->requestor_ == saddr && dbh->requestId_ == requestId) {
				return i;
			}
		}
	}
	return -1;
}

void BroadcastList::onrecv(int idx)
{
	if(idx < 0 || idx >= size_ || list_[idx].idle()) return;
	
	list_[idx].prob() = list_[idx].prob() / 2;
	// if the transmission probability is below threshold
	// drop the packet
	if(list_[idx].prob() < a_->dropBroadcastProb()) {
		list_[idx].drop();
		--count_;
	} else {
		list_[idx].resched(a_->broadcastWait() * 
						Random::uniform() * a_->broadcastDelay());
	}
}

void BroadcastList::commit(Packet *p)
{
	if(full()) return;
	
	for(int i = 0; i < size_; ++i) {
		if(list_[i].idle()) {
			list_[i].putPacket(p);
			list_[i].start(a_->initBroadcastProb());
			++count_;
			return;
		}
	}
}

void BroadcastList::timeout(int uid)
{
	int idx = lookup(uid);
	if(idx < 0) return;
	
	double p = Random::uniform();
	if(p <= list_[idx].prob()) {
		#ifdef DBG_DEBUG
		hdr_cmn *cmh = hdr_cmn::access(list_[idx].getPacket());
		printf("%.5f %d bc-prob %d\n", 
						Scheduler::instance().clock(), a_->addr(), cmh->uid());
		#endif
		DBGAgent::sendCount()++;
		a_->broadcast(list_[idx].getPacket());
		list_[idx].free();
	} else {
		list_[idx].drop();
	}
	--count_;
}

void BroadcastList::cancel(int idx)
{
	if(idx < 0 || idx >= size_ || list_[idx].idle()) return;
	
	list_[idx].drop();
	--count_;
}
