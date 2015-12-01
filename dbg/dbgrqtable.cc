#include "scheduler.h"
#include "dbgrqtable.h"

//-------------- Request -----------------
Request::Request(nsaddr_t requestor, int requestId): 
requestor_(requestor), requestId_(requestId), prevhop_(0), nexthop_(0),
next_(0), time_(Scheduler::instance().clock()), replied_(false)
{}

//-------------- RequestTable ------------
DBGRequestTable::~DBGRequestTable()
{
	Request *r, *q;
	for(r = head_; r != 0; r = q) {
		q = r->next_;
		delete r;
	}
}

Request* DBGRequestTable::lookup(nsaddr_t requestor, int requestId)
{
	Request *r;
	for(r = head_; 
		r != 0 && (r->requestId() != requestId || r->requestor() != requestor); 
		r = r->next_);
	return r;
}

Request* DBGRequestTable::insert(nsaddr_t requestor, int requestId)
{
	Request *r = new Request(requestor, requestId);
	r->next_ = head_;
	head_ = r;
	return r;
}

void DBGRequestTable::remove(nsaddr_t requestor, int requestId)
{
	Request *r, *q;
	for(r = head_, q = 0; r != 0; r = r->next_) {
		if(r->requestId() == requestId && r->requestor() == requestor) {
			if(q == 0) {
				head_ = r->next_;
			} else {
				q->next_ = r->next_;
			}
			delete r;
			return;
		}
	}
}

int DBGRequestTable::count()
{
	int c = 0;
	Request *r;
	for(r = head_; r != 0; r = r->next_) {
		++c;
	}
	return c;
}

bool DBGRequestTable::replied(nsaddr_t requestor, int requestId)
{
	Request *r = lookup(requestor, requestId);
	if(r == 0) return false;
	return r->replied();
}

void DBGRequestTable::setReplied(nsaddr_t requestor, int requestId, bool r)
{
	Request *rq = lookup(requestor, requestId);
	if(rq == 0) return;
	rq->replied() = r;
}
