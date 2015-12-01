#ifndef dbg_bclist_h
#define dbg_bclist_h

#include "agent.h"

class DBGAgent;

class BroadcastWaitTimer : public TimerHandler {
public:
	BroadcastWaitTimer(): a_(0), pkt_(0), prob_(0) {}
	~BroadcastWaitTimer() {}
	
	void setAgent(DBGAgent *a) { a_ = a; }
	void putPacket(Packet *p);
	Packet *getPacket() { return pkt_; }
	void start(double prob);
	bool initialized() { return (a_ != 0 && pkt_ != 0); }
	bool idle() { return (pkt_ == 0); }
	void free();
	void drop();
	double& prob() { return prob_; }
	
protected:
	DBGAgent *a_;
	double prob_;
	Packet *pkt_;
	
	void expire(Event *e);
};

class BroadcastList {
public:
	BroadcastList(int size, DBGAgent *a);
	~BroadcastList();

	int lookup(int uid);
	int lookupRequest(nsaddr_t saddr, int requestId);
	void onrecv(int idx);
	void timeout(int uid);
	void cancel(int idx);
	bool full() { return (count_ == size_); }
	void insert();
	void commit(Packet *p);
	

protected:
	DBGAgent *a_;
	BroadcastWaitTimer *list_;
	int size_, count_;
};

#endif // dbg_bclist_h
