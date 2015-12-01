#ifndef dbg_agent_h
#define dbg_agent_h

#include "ip.h"
#include "agent.h"
#include "grouplist.h"

//#define DBG_DEBUG

#define NOW_TIME				(Scheduler::instance().clock())

class DBGAgent;

class DBGAgent : public Agent {
public:
	DBGAgent(nsaddr_t taddr);
	virtual ~DBGAgent();

	void recv(Packet *p, Handler *h);
	int command(int argc, const char *const* argv);
	
	void send(nsaddr_t daddr, int len);
	void broadcast(Packet *p, bool waitshort = false);
	void join(nsaddr_t groupId);
	Group* declare(nsaddr_t groupId);

	int broadcastWait() { return broadcastWait_; }
	double dropBroadcastProb() { return dropBroadcastProb_; }
	double initBroadcastProb() { return initBroadcastProb_; }
	double broadcastJitter() { return broadcastJitter_; }
	double broadcastDelay() { return broadcastDelay_; }
	double leaveThreshold() { return leaveThreshold_; }
	nsaddr_t addr() { return addr_; }
	bool gateway(nsaddr_t groupId);
	
	void joinTimeout(nsaddr_t groupId);
	void broadcastTimeout(nsaddr_t groupId, int uid);
	void heartbeatTimeout(nsaddr_t groupId);
	void checkTimeout(nsaddr_t groupId);
	void findWaitTimeout(FindRequest *freq);
	
	int recvJoinCount(nsaddr_t groupId);
	bool isgateway(nsaddr_t groupId);
	bool covered(nsaddr_t groupId);
	void printDom(nsaddr_t groupId);
	
	Group *checkGroup(nsaddr_t groupId, bool test = false);
	
	NeighborList *neighbors(nsaddr_t groupId);
	FindRequestList *findTable(nsaddr_t groupId);
	
	static int &sendCount() { return sendCount_; }
	
protected:
	nsaddr_t addr_;
	GroupList groups_;
	static int sendCount_;
	
	// protocol parameters
	double initBroadcastProb_;
	double dropBroadcastProb_;
	int broadcastWait_;
	double broadcastWaitDelay_;
	double broadcastDelay_;
	double broadcastJitter_;
	double requestWaitShort_;
	double requestWaitLong_;
	double heartbeatDelay_;
	double checkDelay_;
	double leaveThreshold_;
	double findWaitDelay_;
	double initServeTime_;
	double serveIncrement_;
	double waitConfirmDelay_;
	double confirmDelay_;
	int broadcastListSize_;
	
	void handleJoinRequest(Packet *p);
	void handleJoinReply(Packet *p);
	void handleJoinConfirm(Packet *p);
	void handleHeartbeat(Packet *p);
	void handleGatewayLeave(Packet *p);
	void handleFindRequest(Packet *p);
	void handleFindReply(Packet *p);
	void handleData(Packet *p);
	void sendData(nsaddr_t groupId, int size);
	void sendJoinRequest(nsaddr_t groupId, int ttl, bool test = false);
	void sendJoinReply(int requestId, nsaddr_t groupId, 
						nsaddr_t requestor, nsaddr_t nexthop);
	void sendTest(int len);
	void sendConfirm(int requestId, nsaddr_t groupId, 
						nsaddr_t replier, nsaddr_t nexthop);
	void sendGatewayLeave(nsaddr_t groupId);
	void sendFindRequest(FindRequest *freq);
	void sendFindReply(nsaddr_t groupId, nsaddr_t requestor, 
						nsaddr_t requestId, nsaddr_t nexthop);
	void forwardFindRequest(Packet *p, FindRequest *freq);
	bool ponder(nsaddr_t groupId);
};

#endif //  dbg_agent_h
