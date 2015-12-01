#ifndef dbg_history_h
#define dbg_history_h

#include "ip.h"

struct HistoryRec {
	int uid_;
	int duid_;
	double latency_;
	HistoryRec *next_;
	HistoryRec(int uid, int duid, double latency): uid_(uid), duid_(duid), next_(0), latency_(latency) {}
};

struct HistoryForNode {
	nsaddr_t saddr_;
	HistoryRec *list_;
	HistoryForNode *next_;
	HistoryForNode(nsaddr_t saddr): saddr_(saddr), list_(0), next_(0) {}
};

class History {
public:
	History();
	~History();

	bool insert(nsaddr_t saddr, int uid, int duid, double latency);
	double avgLatency(nsaddr_t saddr);
	int lookup(nsaddr_t saddr, int duid);
	bool missing(nsaddr_t& saddr, int& duid);
	int count(nsaddr_t saddr);
	int total();

protected:
	HistoryForNode *history_;
	
	HistoryForNode *lookupNode(nsaddr_t saddr);
};

#endif // dbg_history_h
