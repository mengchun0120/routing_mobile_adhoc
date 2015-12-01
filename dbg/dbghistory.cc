#include "dbghistory.h"

History::History():
history_(0)
{
}

History::~History()
{
	HistoryForNode *h, *n;
	for(h = history_; h != 0; h = n) {
		HistoryRec *next, *rec;
		for(rec = h->list_; rec != 0; rec = next) {
			next = rec->next_;
			delete rec;
		}
		n = h->next_;
		delete h;
	}
}

bool History::insert(nsaddr_t saddr, int uid, int duid, double latency)
{	
	HistoryForNode *h = lookupNode(saddr);
	bool record = false;
	if(h == 0) {
		h = new HistoryForNode(saddr);
		h->list_ = new HistoryRec(uid, duid, latency);
		h->next_ = history_;
		history_ = h;
		record = true;
	} else {
		HistoryRec *prev, *rec;
		for(prev = 0, rec = h->list_; rec != 0; rec = rec->next_) {
			if(rec->duid_ == duid || rec->duid_ > duid) {
				break;	// if the packet is found, don't insert it
			}
			prev = rec;
		}

		if(rec == 0 || rec->duid_ > duid) {
			HistoryRec *newrec = new HistoryRec(uid, duid, latency);
			newrec->next_ = rec;
			if(prev == 0) {
				history_->list_ = newrec;
			} else {
				prev->next_ = newrec;
			}
			record = true;
		}
	}
	
	return record;
}

bool History::missing(nsaddr_t& saddr, int& duid)
{
	HistoryForNode *h;
	HistoryRec *rec;;
	for(h = history_; h != 0; h = h->next_) {
		int d = -1;
		for(rec = h->list_; rec != 0; rec = rec->next_) {
			if(d + 1 < rec->duid_) {
				saddr = h->saddr_;
				duid = d + 1;
				return true;
			}
			d = rec->duid_;
		}
	}
	return false;
}

// return uid
int History::lookup(nsaddr_t saddr, int duid)
{
	HistoryForNode *h = lookupNode(saddr);
	if(h == 0) return -1;
	
	HistoryRec *rec;
	for(rec = h->list_; rec != 0 && rec->duid_ != duid; rec = rec->next_);
	
	return (rec == 0) ? -1 : rec->uid_;
}

int History::count(nsaddr_t saddr)
{
	HistoryForNode *h = lookupNode(saddr);
	if(h == 0) return 0;
	int c = 0;
	for(HistoryRec *rec = h->list_; rec != 0; rec = rec->next_) {
		++c;
	}
	return c;
}

int History::total()
{
	int count = 0;
	for(HistoryForNode *h = history_; h != 0; h = h->next_) {
		for(HistoryRec *rec = h->list_; rec != 0; rec = rec->next_) {
			++count;
		}	
	}
	return count;
}

HistoryForNode* History::lookupNode(nsaddr_t saddr)
{
	HistoryForNode *h;
	for(h = history_; h != 0 && h->saddr_ != saddr; h = h->next_);
	return h;
}

double History::avgLatency(nsaddr_t saddr)
{
	HistoryForNode *h = lookupNode(saddr);
	if(h == 0) return 0;
	double lat = 0;
	int count = 0;
	for(HistoryRec *rec = h->list_; rec != 0; rec = rec->next_) {
		lat += rec->latency_;
		++count;
	}
	return (lat / count);
}