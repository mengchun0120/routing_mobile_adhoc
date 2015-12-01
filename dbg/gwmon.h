#ifndef dbg_gwmon_h
#define dbg_gwmon_h

class DBGAgent;

#define GATEWAY_COUNT						200

class Gateway {
friend class GatewayList;
friend class GatewayMonitor;
public:
	Gateway(): a_(0), connected_(false), enum_(false) {}
	Gateway(DBGAgent *a): a_(a), connected_(false), enum_(false) {}
	~Gateway() {}
	
	Gateway& operator=(Gateway& gw)
	{
		a_ = gw.a_;
		connected_ = gw.connected_;
		enum_ = gw.enum_;
		return *this;
	}
	
protected:
	DBGAgent *a_;
	bool connected_;
	bool enum_;
};

class GatewayList {
friend class GatewayMonitor;
public:
	GatewayList(nsaddr_t groupId);
	~GatewayList();
	
	int add(DBGAgent *a);
	void remove(DBGAgent *a);
	int lookup(nsaddr_t addr);
	int count() { return count_; }
	const char *list();
	bool connected();
	Gateway* index(int i);
	
protected:
	Gateway *list_;
	int count_;
	nsaddr_t groupId_;
	GatewayList *next_;
	
	void mark(int idx);
};

class GatewayMonitor {
public:
	GatewayMonitor(): head_(0) {}
	~GatewayMonitor();
	
	static GatewayMonitor& instance() { return *monitor_; }
	GatewayList *lookup(nsaddr_t groupId);
	GatewayList *add(nsaddr_t groupId);
	GatewayList *declare(nsaddr_t groupId);
	Gateway *addGateway(nsaddr_t groupId, DBGAgent *a);
	Gateway *lookupGateway(nsaddr_t groupId, DBGAgent *a);
	void removeGateway(nsaddr_t groupId, DBGAgent *a);
	int count(nsaddr_t groupId);
	const char *list(nsaddr_t groupId);
	bool connected(nsaddr_t groupId);
	
protected:
	GatewayList *head_;
	static GatewayMonitor *monitor_;
};

#endif // dbg_gwmon_h
