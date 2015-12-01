#include "dbghdr.h"
#include "gwmon.h"
#include "dbgagent.h"

GatewayMonitor* GatewayMonitor::monitor_ = new GatewayMonitor();

//--------------- GatewayList ---------------
GatewayList::GatewayList(nsaddr_t groupId): 
groupId_(groupId), count_(0), next_(0)
{
	list_ = new Gateway[GATEWAY_COUNT];
}

GatewayList::~GatewayList()
{
	delete[] list_;
}

Gateway* GatewayList::index(int idx)
{
	if(idx < 0 || idx >= count_) return 0;
	return &list_[idx];
}

int GatewayList::add(DBGAgent *a)
{
	int i;
	for(i = 0; i < count_; ++i) {
		if(list_[i].a_->addr() == a->addr()) {
			return i;
		} else if(list_[i].a_->addr() > a->addr()) {
			break;
		}
	}
	for(int j = count_ - 1; j >= i; --j) {
		list_[j + 1] = list_[j];
	}
	list_[i].a_ = a;
	++count_;
	return i;
}

void GatewayList::remove(DBGAgent *a)
{
	int idx = lookup(a->addr());
	if(idx < 0) return;
	for(int i = idx + 1; i < count_; ++i) {
		list_[i - 1] = list_[i];
	}
	--count_;
}

int GatewayList::lookup(nsaddr_t addr)
{
	int left, right, middle;
	left = 0;
	right = count_ - 1;
	while(left <= right) {
		middle = (left + right) >> 1;
		if(list_[middle].a_->addr() == addr) {
			return middle;
		} else if(list_[middle].a_->addr() > addr) {
			right = middle - 1;
		} else {
			left = middle + 1;
		}
	}
	return -1;
}

const char* GatewayList::list()
{
	char buffer[500];
	int c = 0;
	buffer[0] = 0;
	int i;
	for(i = 0; i < count_; ++i) {
		c += sprintf(buffer + c, " %d", list_[i].a_->addr());
	}
	return buffer;
}

void GatewayList::mark(int i)
{
	list_[i].enum_ = true;
	list_[i].connected_ = true;
	NeighborList *neighbors = list_[i].a_->neighbors(groupId_);
	int idx;
	for(Neighbor *n = neighbors->first(); n != 0; n = neighbors->next()) {
		if(n->connected()) {
			idx = lookup(n->addr());
			if(idx >= 0) {
				list_[idx].connected_ = true;
			}
		}
	}
}

bool GatewayList::connected()
{
	if(count_ == 0) return false;
	
	// initialize the list first
	int i;
	for(i = 0; i < count_; ++i) {
		list_[i].connected_ = false;
		list_[i].enum_ = false;
	}
	
	mark(0);
	int cc = 1;
	while(true) {
		for(i = 1; i < count_; ++i) {
			if(list_[i].enum_ == false && list_[i].connected_ == true) {
				break;
			}
		}
		
		if(i >= count_) break;
		mark(i);
		++cc;
	}
	
	return (cc == count_);
}
//---------------- GatewayMonitor -----------
GatewayMonitor::~GatewayMonitor()
{
	GatewayList *p, *q;
	for(p = head_; p != 0; p = q) {
		q = p->next_;
		delete p;
	}
}

GatewayList* GatewayMonitor::lookup(nsaddr_t groupId)
{
	GatewayList *p;
	for(p = head_; p != 0 && p->groupId_ != groupId; p = p->next_);
	return p;
}

GatewayList* GatewayMonitor::add(nsaddr_t groupId)
{
	GatewayList *p = new GatewayList(groupId);
	p->next_ = head_;
	head_ = p;
	return p;
}

GatewayList* GatewayMonitor::declare(nsaddr_t groupId)
{
	GatewayList *p = lookup(groupId);
	if(p == 0) p = add(groupId);
	return p;
}

Gateway* GatewayMonitor::addGateway(nsaddr_t groupId, DBGAgent *a)
{
	GatewayList *p = lookup(groupId);
	if(p == 0) return 0;
	return p->index(p->add(a));
}

void GatewayMonitor::removeGateway(nsaddr_t groupId, DBGAgent *a)
{
	GatewayList *p = lookup(groupId);
	if(p == 0) return;
	p->remove(a);
}

Gateway* GatewayMonitor::lookupGateway(nsaddr_t groupId, DBGAgent *a)
{
	GatewayList *p = lookup(groupId);
	if(p == 0) return 0;
	return p->index(p->lookup(a->addr()));
}

int GatewayMonitor::count(nsaddr_t groupId)
{
	GatewayList *p = lookup(groupId);
	return (p != 0) ? p->count() : 0;
}

const char* GatewayMonitor::list(nsaddr_t groupId)
{
	GatewayList *p = lookup(groupId);
	return (p != 0) ? p->list() : "";
}

bool GatewayMonitor::connected(nsaddr_t groupId)
{
	GatewayList *p = lookup(groupId);
	return (p != 0) ? p->connected() : false;
}
