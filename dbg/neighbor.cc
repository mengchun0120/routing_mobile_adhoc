#include "dbghdr.h"
#include "neighbor.h"
#include "findreq.h"
#include "dbgagent.h"

//------------------ Neighbor --------------
Neighbor::Neighbor(nsaddr_t addr):
addr_(addr), time_(NOW_TIME), size_(0),
next_(0), connected_(true), count_(0), neighbors_(0)
{
}

Neighbor::~Neighbor()
{
	delete[] neighbors_;
}

void Neighbor::setNeighbors(nsaddr_t *list, int size)
{
	if(size > size_) {
		delete[] neighbors_;
		neighbors_ = new nsaddr_t[size];
		size_ = size;
	}
	
	count_ = size;
	for(int i = 0; i < size; ++i)
		neighbors_[i] = list[i];
}

int Neighbor::lookup(nsaddr_t addr)
{
	for(int i = 0; i < count_; ++i) {
		if(neighbors_[i] == addr) return i;
	}
	return -1;
}

void Neighbor::remove(nsaddr_t addr)
{
	int idx = lookup(addr);
	if(idx < 0) return;
	for(int i = idx + 1; i < count_; ++i) {
		neighbors_[i - 1] = neighbors_[i];
	}
	--count_;
}

//----------------- NeighborList -----------
NeighborList::NeighborList(DBGAgent *a, nsaddr_t groupId):
a_(a), head_(0), groupId_(groupId), cursor_(0)
{
}

NeighborList::~NeighborList()
{
	Neighbor *p, *q;
	for(p = head_; p != 0; p = q) {
		q = p->next_;
		delete p;
	}
}

int NeighborList::count()
{
	int cnt = 0;
	Neighbor *p;
	for(p = head_; p != 0; p = p->next_)
		if(p->connected()) ++cnt;
	return cnt;
}

int NeighborList::fill(nsaddr_t *list, int size)
{
	int i = 0;
	Neighbor *p;
	for(p = head_; p != 0 && i < size; p = p->next_) {
		if(p->connected()) list[i++] = p->addr();
	}
	return i;
}

Neighbor* NeighborList::lookup(nsaddr_t addr)
{
	Neighbor *p;
	for(p = head_; p != 0; p = p->next_) {
		if(p->connected() && p->addr() == addr) {
			return p;
		}
	}
	return 0;
}

Neighbor* NeighborList::lookupThrough(nsaddr_t addr)
{
	Neighbor *p;
	for(p = head_; p != 0; p = p->next_)
		if(p->connected() && (p->addr() == addr || p->lookup(addr) >= 0) ) 
			return p;
	return 0;
}

Neighbor* NeighborList::add(nsaddr_t addr)
{
	Neighbor *p;

	for(p = head_; p != 0; p = p->next_) {
		if(addr == p->addr()) {
			p->connected() = true;
			p->time() = NOW_TIME;
			return p;
		}
	}
	Neighbor *r = new Neighbor(addr);
	r->next_ = head_;
	head_ = r;
	return r;
}

void NeighborList::remove(nsaddr_t addr)
{
	Neighbor *p, *q;

	for(p = head_, q = 0; p != 0; p = p->next_) {
		if(p->addr() == addr) {
			if(q == 0) {
				head_ = p->next_;
			} else {
				q->next_ = p->next_;
			}
			delete p;
			return;
		}
		q = p;
	}
}

int NeighborList::checkFind(FindRequest *freq)
{
	Neighbor *p, *n;
	int cc = 0;
	nsaddr_t *nl;
	FindEntry *fe;

	for(p = head_; p != 0; p = p->next_) {
		if(p->connected() || lookupThrough(p->addr()) != 0) continue;
			
		++cc;
		fe = freq->addFind(p->addr());
			
		nl = p->neighbors();
		for(int i = 0; i < p->neighborCount(); ++i)
			if(nl[i] != a_->addr())
				fe->addSub(nl[i]);
	}

	return cc;
}

// return true if the node can be unmarked according to rule 1.
bool NeighborList::ponderClassOne()
{
	Neighbor *p, *q;
	for(p = head_; p != 0; p = p->next_) {
		if(p->connected() && a_->addr() < p->addr()) {
			if(p->containClose(a_->addr()) == false) continue;
			for(q = head_; 
					q != 0 && q->connected() && p->containClose(q->addr());
					q = q->next_);
			if(q == 0) return true;
		}
	}
	return false;
}

// return true if the node can be unmarked according to rule 2
bool NeighborList::ponderClassTwo()
{
	Neighbor *p, *q, *r;
	for(p = head_; p != 0; p = p->next_) {
		if(p->connected() && a_->addr() < p->addr()) {
			for(q = p->next_; q != 0; q = q->next_) {
				if(q->connected() && a_->addr() < q->addr()) {
					for(r = head_; 
							r != 0 && r->connected() && ( p->containOpen(r->addr()) || q->containOpen(r->addr()) ); 
							r = r->next_);
					if(r == 0) return true;
				}
			}
		}
	}
	return false;
}

const char* NeighborList::list()
{
	char buffer[500];
	buffer[0] = 0;
	int c = 0;
	Neighbor *p;
	for(p = head_; p != 0; p = p->next_) {
		if(p->connected()) {
			c += sprintf(buffer + c, " %d:", p->addr());
			nsaddr_t *nlist = p->neighbors();
			for(int i = 0; i < p->neighborCount(); ++i) {
				c += sprintf(buffer + c, "%d,", nlist[i]);
			}
		}
	}
	return buffer;
}

void NeighborList::flush()
{
	Neighbor *p, *q, *r;
	p = head_;
	q = 0;
	for(p = head_, q = 0; p != 0; p = r) {
		r = p->next_;
		if(p->connected() == false) {
			if(q == 0) {
				head_ = r;
			} else {
				q->next_ = r;
			}
			delete p;
		} else {
			q = p;
		}
	}
}

void NeighborList::purge(nsaddr_t addr)
{
	Neighbor *p;
	for(p = head_; p != 0; p = p->next_) {
		p->remove(addr);
	}
}

// return the total count of disconnected nodes
int NeighborList::check()
{
	Neighbor *p;
	int c2 = 0;
	for(p = head_; p != 0; p = p->next_) {
		if(p->connected()) {
			if(NOW_TIME - p->time() > a_->leaveThreshold()) {
				#ifdef DBG_DEBUG
				if(a_->gateway(groupId_)) 
					printf("%.5f %d detach gid=%d addr=%d time=%.5f\n", NOW_TIME, a_->addr(), groupId_, p->addr(), p->time());
				#endif
				p->connected() = false;
				++c2;
			}
		} else {
			++c2;
		}
	}
	return c2;
}

// initialize the iterator
Neighbor* NeighborList::first()
{
	cursor_ = head_;
	return cursor_;
}

// move the cursor to next neighbor
Neighbor* NeighborList::next()
{
	if(cursor_ != 0) {
		cursor_ = cursor_->next_;
	}
	return cursor_;
}
