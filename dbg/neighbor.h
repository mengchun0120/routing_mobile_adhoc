#ifndef dbg_neighbor_h
#define dbg_neighbor_h

class DBGAgent;
class FindRequest;

class Neighbor {
friend class NeighborList;
public:
	Neighbor(nsaddr_t addr);
	~Neighbor();
	
	nsaddr_t& addr() { return addr_; }
	double& time() { return time_; }
	bool& connected() { return connected_; }
	int neighborCount() { return count_; }
	void setNeighbors(nsaddr_t *list, int size);
	nsaddr_t *neighbors() { return neighbors_; }
	int lookup(nsaddr_t addr);
	void remove(nsaddr_t addr);
	bool containClose(nsaddr_t addr) { return (addr_ == addr || lookup(addr) >= 0); }
	bool containOpen(nsaddr_t addr) { return (lookup(addr) >= 0); }
	
protected:
	Neighbor *next_;
	nsaddr_t addr_;
	double time_;
	bool connected_;
	int count_;
	int size_;
	nsaddr_t *neighbors_;
};

class NeighborList {
public:
	NeighborList(DBGAgent *a, nsaddr_t groupId);
	~NeighborList();
	
	int count();
	const char *list();
	
//	void setPacket(Packet *p);
	Neighbor *lookup(nsaddr_t addr);
	Neighbor *lookupThrough(nsaddr_t addr);
	Neighbor *add(nsaddr_t addr);
	void remove(nsaddr_t addr);
	void purge(nsaddr_t addr);
	bool ponderClassOne();
	bool ponderClassTwo();
	void flush();
	int fill(nsaddr_t *list, int size);
	int check();
	int checkFind(FindRequest *freq);
	Neighbor *first();
	Neighbor *next();
	
private:
	Neighbor *head_;
	DBGAgent *a_;
	nsaddr_t groupId_;
	Neighbor *cursor_;
};

#endif // dbg_neighbor_h
