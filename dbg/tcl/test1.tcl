Agent/DBGAgent set initBroadcastProb_		0.35
Agent/DBGAgent set dropBroadcastProb_		0.1
Agent/DBGAgent set broadcastWait_ 			5
Agent/DBGAgent set broadcastWaitDelay_		0.025
Agent/DBGAgent set broadcastDelay_ 			0.02
Agent/DBGAgent set broadcastJitter_			0.01
Agent/DBGAgent set heartbeatDelay_			0.1
Agent/DBGAgent set broadcastListSize_		400
Agent/DBGAgent set requestWaitShort_		0.1
Agent/DBGAgent set requestWaitLong_			4
Agent/DBGAgent set checkDelay_				0.05
Agent/DBGAgent set leaveThreshold_			0.3

remove-packet-header PGM PGM_SPM PGM_NAK Pushback NV LDP MPLS rtProtoLS Ping
remove-packet-header TFRC TFRC_ACK Diffusion RAP TORA IMEP MIP
remove-packet-header IPinIP Encap HttpInval MFTP SRMEXT SRM aSRM
remove-packet-header mcastCtrl CtrMcast rtProtoDV GAF Snoop TCPA TCP IVS
remove-packet-header Resv UMP Src_rt
remove-packet-header AODV SR

Phy/WirelessPhy set RXThresh_ 3.65262e-10

set val(chan)	 	Channel/WirelessChannel
set val(prop) 		Propagation/TwoRayGround
set val(netif) 		Phy/WirelessPhy
set val(mac) 		Mac/802_11
set val(ifq) 		Queue/DropTail/PriQueue
set val(ll)			LL
set val(ant)		Antenna/OmniAntenna
set val(ifqlen)		100
set val(nn)			100
set val(rp)			DBG
set val(x)			1000
set val(y)			1000
set val(stop)		100

set path /home/mengchun/ns-allinone-2.29/ns-2.29/dbg

set ns_ [new Simulator]
set tracefd [open "$path/trace/test1.tr" w]
set recvfd [open "$path/trace/recv1.tr" w]
$ns_ trace-all $tracefd

set topo [new Topography]
$topo load_flatgrid $val(x) $val(y)

set god_ [create-god $val(nn)]

set chan_1_ [new $val(chan)]

$ns_ node-config -adhocRouting $val(rp) \
									-llType $val(ll) \
									-macType $val(mac) \
									-ifqType $val(ifq) \
									-ifqLen $val(ifqlen) \
									-antType $val(ant) \
									-propType $val(prop) \
									-phyType $val(netif) \
									-topoInstance $topo \
									-channel $chan_1_ \
									-agentTrace ON \
									-routerTrace ON \
									-macTrace OFF \
									-movementTrace OFF

for {set i 0} {$i < $val(nn)} {incr i} {
	set node_($i) [$ns_ node]
	set agent_($i) [new Agent/DBGAgent [$node_($i) node-addr]]
	$ns_ attach-agent $node_($i) $agent_($i)
}

source "$path/scen/scen-s10"

set t 10
for {set i 0} {$i < 2} {incr i 2} {
	$ns_ at $t "$agent_($i) join-group 0"
	set t [expr $t+1]
}

proc getRecvCount {} {
	global agent_ val
	set count 0
	for {set i 0} {$i < $val(nn)} {incr i} {
		set k [$agent_($i) recv-join-count 0]
		set count [expr $count + $k]
	}
	return $count
}

$ns_ at 10.0 "recRecvCount"

proc recRecvCount {} {
	global ns_ recvfd
	puts $recvfd "[$ns_ now] [getRecvCount]"
	$ns_ after 1.0 "recRecvCount"
}

proc stop {} {
	global ns_ tracefd agent_ val recvfd
	$ns_ flush-trace
	close $tracefd
	for {set i 0} {$i < $val(nn)} {incr i} {
		set k [$agent_($i) recv-join-count 0]
		puts $recvfd "$i recv $k"
	}
	exit 0
}

$ns_ at $val(stop).02 "stop"
$ns_ run
