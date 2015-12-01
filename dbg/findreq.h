#ifndef dbg_findreq_h
#define dbg_findreq_h

#include "timer-handler.h"

class DBGAgent;

class FindEntry {
friend class FindRequest;
public:
	FindEntry(nsaddr_t addr): addr_(addr), find_(false), next_(0), sub_(0) {}
	~FindEntry();
	
	nsaddr_t& addr() { return addr_; }
	bool& find() { return find_; }
	void addSub(nsaddr_t addr);
	bool subEmpty() { return (sub_ == 0); }
	
protected:
	nsaddr_t addr_;
	bool find_;
	FindEntry *sub_;
	FindEntry *next_;
};

class FindRequest : public TimerHandler {
friend class FindRequestList;
public:
	FindRequest(DBGAgent *a, nsaddr_t groupId, nsaddr_t requestor, int requestId);
	~FindRequest();

	nsaddr_t& requestor() { return requestor_; }
	int& requestId() { return requestId_; }
	nsaddr_t& prevhop() { return prevhop_; }
	nsaddr_t groupId() { return groupId_; }
	int findCount(bool find);
	bool finished();

	FindEntry *addFind(nsaddr_t addr);
	FindEntry *lookupFind(nsaddr_t addr);
	int fillFind(nsaddr_t *list, int count, bool find);
	bool subEmpty();
	FindRequest *distractSub(int requestId);
	
protected:
	DBGAgent *a_;
	nsaddr_t groupId_;
	nsaddr_t requestor_;
	int requestId_;
	nsaddr_t prevhop_;
	FindEntry *findList_;
	FindRequest *next_;
	
	void expire(Event *e);
};

class FindRequestList {
public:
	FindRequestList(DBGAgent *a, nsaddr_t groupId);
	~FindRequestList();

	FindRequest *add(nsaddr_t requestor, int requestId);
	FindRequest *lookup(nsaddr_t requestor, int requestId);
	FindRequest *addMarked(nsaddr_t requestor, int requestId, nsaddr_t *list, 
											int count, bool& reply, nsaddr_t prevhop);
	void finish(nsaddr_t addr);
	bool finished();
	
protected:
	DBGAgent *a_;
	nsaddr_t groupId_;
	FindRequest *list_;
};

#endif // dbg_findreq_h
