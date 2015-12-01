#include "grouplist.h"
#include "dbgagent.h"

//--------------- JoinRequestTimer ----
void JoinRequestTimer::expire(Event *e)
{
	a_->joinTimeout(groupId_);
}

//--------------- HeartBeatTimer -----
void HeartBeatTimer::expire(Event *e)
{
	a_->heartbeatTimeout(groupId_);
}

//--------------- CheckTimer ---------
void CheckTimer::expire(Event *e)
{
	a_->checkTimeout(groupId_);
}


//--------------- Group -------------
Group::Group(nsaddr_t groupId, int broadcastListSize, DBGAgent *a, bool test):
groupId_(groupId), dflag_(false), requestTable_(), findTable_(a, groupId),
joinNear_(false), requestId_(0), next_(0), neighbors_(a, groupId), history_(),
broadcastList_(broadcastListSize, a), join_(false), permanent_(false), find_(false),
joinTimer_(a, groupId), hbTimer_(a, groupId), checkTimer_(a, groupId), dataId_(0)
{
	if(!test) checkTimer_.resched(0);
}

//--------------- GroupList ---------
GroupList::~GroupList()
{
	Group *g, *q;
	for(g = head_; g != 0; g = q) {
		q = g->next_;
		delete g;
	}
}
	
Group* GroupList::lookup(nsaddr_t groupId)
{
	Group *g;
	for(g = head_; g != 0 && g->groupId_ != groupId; g = g->next_);
	return g;
}

Group* GroupList::add(nsaddr_t groupId, int broadcastListSize, bool test)
{
	Group *g = new Group(groupId, broadcastListSize, a_, test);
	g->next_ = head_;
	head_ = g;
	return g;
}

