#ifndef dbg_grouplist_h
#define dbg_grouplist_h

#include "dbghdr.h"
#include "dbghistory.h"
#include "dbgrqtable.h"
#include "dbgbclist.h"
#include "neighbor.h"
#include "findreq.h"

class JoinRequestTimer : public TimerHandler {
public:
	JoinRequestTimer(DBGAgent *a, nsaddr_t groupId): a_(a), groupId_(groupId) {}
	~JoinRequestTimer() {}
	
protected:
	DBGAgent *a_;
	nsaddr_t groupId_;
	void expire(Event *e);
};

class HeartBeatTimer : public TimerHandler {
public:
	HeartBeatTimer(DBGAgent *a, nsaddr_t groupId): a_(a), groupId_(groupId) {}
	~HeartBeatTimer() {}

protected:
	DBGAgent *a_;
	nsaddr_t groupId_;
	void expire(Event *e);
};

class CheckTimer : public TimerHandler {
public:
	CheckTimer(DBGAgent *a, nsaddr_t groupId): a_(a), groupId_(groupId) {}
	~CheckTimer() {}

protected:
	DBGAgent *a_;
	nsaddr_t groupId_;
	void expire(Event *e);
};

class Group {
friend class GroupList;
public:
	nsaddr_t groupId_;
	bool dflag_;
	DBGRequestTable requestTable_;
	BroadcastList broadcastList_;
	JoinRequestTimer joinTimer_;
	HeartBeatTimer hbTimer_;
	CheckTimer checkTimer_;
	History history_;
	NeighborList neighbors_;
	bool joinNear_;
	bool join_;
	bool permanent_;
	bool find_;
	int requestId_;
	int dataId_;
	double serveTime_;
	double serveThreshold_;
	double lastConfirmTime_;
	FindRequestList findTable_;
	
	Group(nsaddr_t groupId, int broadcastListSize, DBGAgent *a, bool test = false);
	~Group() {}
	
private:
	Group *next_;
};

class GroupList {
public:
	GroupList(DBGAgent *a): head_(0), a_(a) {}
	~GroupList();
	
	Group *lookup(nsaddr_t groupId);
	Group *add(nsaddr_t groupId, int broadcastListSize, bool test = false);
protected:
	DBGAgent *a_;
	Group *head_;
};

#endif // dbg_grouplist_h
