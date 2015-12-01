#ifndef dbg_rqtable_h
#define dbg_rqtable_h

class Request {
friend class DBGRequestTable;
public:
	Request(nsaddr_t requestor, int requestId);
	~Request() {}
	
	nsaddr_t& requestor() { return requestor_; }
	nsaddr_t& prevhop() { return prevhop_; }
	nsaddr_t& nexthop() { return nexthop_; }
	int& requestId() { return requestId_; }
	double& time() { return time_; }
	bool& replied() { return replied_; }
	
private:	
	Request *next_;
	nsaddr_t requestor_;
	int requestId_;
	nsaddr_t prevhop_;
	nsaddr_t nexthop_;
	double time_;
	bool replied_;
};

class DBGRequestTable {
public:
	DBGRequestTable(): head_(0) {}
	~DBGRequestTable();
	
	Request* lookup(nsaddr_t requestor, int requestId);
	Request* insert(nsaddr_t requestor, int requestId);
	void remove(nsaddr_t requestor, int requestId);
	int count();
	bool replied(nsaddr_t requestor, int requestId);
	void setReplied(nsaddr_t requestor, int requestId, bool r);

protected:
	Request *head_;
};

#endif // dbg_rqtable_h
