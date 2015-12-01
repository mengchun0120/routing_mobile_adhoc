#include "dbghdr.h"
#include "findreq.h"
#include "neighbor.h"
#include "dbgagent.h"

//---------------- FindEntry -----------------
FindEntry::~FindEntry()
{
	FindEntry *f, *f1;
	for(f = sub_; f != 0; f = f1) {
		f1 = f->next_;
		delete f;
	}
}

void FindEntry::addSub(nsaddr_t addr)
{
	FindEntry *f = new FindEntry(addr);
	f->next_ = sub_;
	sub_ = f;
}

//---------------- FindRequest -----------------
FindRequest::FindRequest(DBGAgent *a, nsaddr_t groupId, nsaddr_t requestor, int requestId):
a_(a), groupId_(groupId), requestor_(requestor), requestId_(requestId), 
prevhop_(0), findList_(0), next_(0)
{
}

FindRequest::~FindRequest()
{	
	FindEntry *f, *f1;
	for(f = findList_; f != 0; f = f1) {
		f1 = f->next_;
		delete f;
	}
}

bool FindRequest::subEmpty()
{
	FindEntry *f;
	for(f = findList_; f != 0; f = f->next_) {
		if(f->subEmpty() == false) return false;
	}
	return true;
}

FindRequest* FindRequest::distractSub(int requestId)
{
	FindRequest *freq = new FindRequest(a_, groupId_, requestor_, requestId);
	FindEntry *f, *f1;
	for(f = findList_; f != 0; f = f->next_) {
		if(f->subEmpty() == false) {
			if(freq->findList_ == 0) {
				freq->findList_ = f->sub_;
			} else {
				for(f1 = freq->findList_; f1->next_ != 0; f1 = f1->next_);
				f1->next_ = f->sub_;
			}
			f->sub_ = 0;
		}
	}
	freq->next_ = next_;
	next_ = freq;
	return freq;
}

int FindRequest::findCount(bool find)
{
	FindEntry *f;
	int cc = 0;
	for(f = findList_; f != 0; f = f->next_) {
		if(f->find() == find) ++cc;
	}
	return cc;
}

bool FindRequest::finished()
{
	FindEntry *f;
	for(f = findList_; f != 0; f = f->next_) {
		if(f->find() == false) return false;
	}
	return true;
}

// add an empty find-list
FindEntry* FindRequest::addFind(nsaddr_t addr)
{
	FindEntry *f = new FindEntry(addr);
	f->next_ = findList_;
	findList_ = f;
	return f;
}

FindEntry* FindRequest::lookupFind(nsaddr_t addr)
{
	FindEntry *f;
	for(f = findList_; f != 0 && f->addr() != addr; f = f->next_);
	return f;
}

void FindRequest::expire(Event *e)
{
	a_->findWaitTimeout(this);
}

// fill found nodes into a list
int FindRequest::fillFind(nsaddr_t *list, int count, bool find)
{
	FindEntry *fe;
	int i;
	
	for(fe = findList_, i = 0; fe != 0 && i < count; fe = fe->next_) {
		if(fe->find() == find) list[i++] = fe->addr();
	}
	
	return i;
}

//--------------- FindRequestList ---------------
FindRequestList::FindRequestList(DBGAgent *a, nsaddr_t groupId):
a_(a), groupId_(groupId), list_(0)
{
}

FindRequestList::~FindRequestList()
{
	FindRequest *c, *c1;
	for(c = list_; c != 0; c = c1) {
		c1 = c->next_;
		delete c;
	}
}

FindRequest* FindRequestList::add(nsaddr_t requestor, int requestId)
{
	FindRequest *c = new FindRequest(a_, groupId_, requestor, requestId);
	c->next_ = list_;
	list_ = c;
	return c;
}

FindRequest* FindRequestList::lookup(nsaddr_t requestor, int requestId)
{
	FindRequest *c;
	for(c = list_; c != 0; c = c->next_) {
		if(c->requestor() == requestor && c->requestId() == requestId) return c;
	}
	return 0;
}

// this function will check if there is any node that can be found in list
// and it will also record the find-request
FindRequest* FindRequestList::addMarked(nsaddr_t requestor, int requestId, 
																	nsaddr_t *list, int count, bool& reply, nsaddr_t prevhop)
{
	FindRequest *f = add(requestor, requestId);
	FindEntry *fe;
	
	reply = false;
	for(int i = 0; i < count; ++i) {
		fe = f->addFind(list[i]);
		if(list[i] == a_->addr()) {
			reply = true;
			fe->find() = true;
		}
	}
	f->prevhop() = prevhop;
	
	return f;
}

bool FindRequestList::finished()
{
	FindRequest *f;
	for(f = list_; f != 0 && f->status() == TIMER_IDLE; f = f->next_);
	return (f == 0);
}

void FindRequestList::finish(nsaddr_t addr)
{
	FindRequest *f;
	FindEntry *fe;
	
	for(f = list_; f != 0; f = f->next_) {
		fe = f->lookupFind(addr);
		if(fe != 0 && fe->find() == false) {
			fe->find() = true;
			if(f->finished()) f->force_cancel();
		}
	}
}