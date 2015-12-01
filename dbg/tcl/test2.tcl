Agent/DBGAgent set initBroadcastProb_		0.7
Agent/DBGAgent set dropBroadcastProb_		0.2
Agent/DBGAgent set broadcastWait_ 			5
Agent/DBGAgent set broadcastWaitDelay_		0.025
Agent/DBGAgent set broadcastDelay_ 			0.02
Agent/DBGAgent set broadcastJitter_			0.01
Agent/DBGAgent set heartbeatDelay_			0.1
Agent/DBGAgent set broadcastListSize_		400
Agent/DBGAgent set requestWaitShort_		0.1
Agent/DBGAgent set requestWaitLong_			4


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
set val(nn)			10
set val(rp)			DBG
set val(x)			1000
set val(y)			1000
set val(stop)		100

set path /home/mengchun/ns-allinone-2.29/ns-2.29/dbg

set ns_ [new Simulator]
set tracefd [open "$path/trace/test2.tr" w]
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
	$node_($i) random-motion 0
}

$node_(0) set X_ 10
$node_(0) set Y_ 10
$node_(0) set Z_ 0
$node_(1) set X_ 110
$node_(1) set Y_ 110
$node_(1) set Z_ 0
$node_(2) set X_ 210
$node_(2) set Y_ 210
$node_(2) set Z_ 0
$node_(3) set X_ 310
$node_(3) set Y_ 310
$node_(3) set Z_ 0
$node_(4) set X_ 160
$node_(4) set Y_ 60
$node_(4) set Z_ 0
$node_(5) set X_ 60
$node_(5) set Y_ 160
$node_(5) set Z_ 0
$node_(6) set X_ 230
$node_(6) set Y_ 170
$node_(6) set Z_ 0
$node_(7) set X_ 140
$node_(7) set Y_ 270
$node_(7) set Z_ 0
$node_(8) set X_ 310
$node_(8) set Y_ 10
$node_(8) set Z_ 0
$node_(9) set X_ 215
$node_(9) set Y_ 90
$node_(9) set Z_ 0

$ns_ at 10.0 "$agent_(3) declare-group 0"
$ns_ at 11.0 "$agent_(0) join-group 0"
$ns_ at 12.0 "$agent_(8) join-group 0"

proc stop {} {
	global ns_ tracefd
	$ns_ flush-trace
	close $tracefd
	exit 0
}

$ns_ at $val(stop).02 "stop"
$ns_ run
