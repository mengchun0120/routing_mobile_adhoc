# simulation script for 863 project
# transmission range: 25m
Agent/DBGAgent set initBroadcastProb_		0.8
Agent/DBGAgent set dropBroadcastProb_		0.1
Agent/DBGAgent set broadcastWait_ 			5
Agent/DBGAgent set broadcastWaitDelay_	0.025
Agent/DBGAgent set broadcastDelay_ 			0.02
Agent/DBGAgent set broadcastJitter_			0.01
Agent/DBGAgent set heartbeatDelay_			0.3
Agent/DBGAgent set broadcastListSize_		400
Agent/DBGAgent set requestWaitShort_		0.1
Agent/DBGAgent set requestWaitLong_			2
Agent/DBGAgent set checkDelay_					0.1
Agent/DBGAgent set leaveThreshold_			1
Agent/DBGAgent set findWaitDelay_				0.6
Agent/DBGAgent set initServeTime_				5
Agent/DBGAgent set serveIncrement_			2.3
Agent/DBGAgent set waitConfirmDelay_		0.6
Agent/DBGAgent set confirmDelay_				2

remove-packet-header PGM PGM_SPM PGM_NAK Pushback NV LDP MPLS rtProtoLS Ping
remove-packet-header TFRC TFRC_ACK Diffusion RAP TORA IMEP MIP
remove-packet-header IPinIP Encap HttpInval MFTP SRMEXT SRM aSRM
remove-packet-header mcastCtrl CtrMcast rtProtoDV GAF Snoop TCPA TCP IVS
remove-packet-header Resv UMP Src_rt
remove-packet-header AODV SR

Phy/WirelessPhy set RXThresh_ 3.07645e-07

set val(chan)	 	Channel/WirelessChannel
set val(prop) 		Propagation/TwoRayGround
set val(netif) 		Phy/WirelessPhy
set val(mac) 		Mac/802_11
set val(ifq) 		Queue/DropTail/PriQueue
set val(ll)			LL
set val(ant)		Antenna/OmniAntenna
set val(ifqlen)		100
set val(nn)			80
set val(rp)			DBG
set val(x)			100
set val(y)			500
set val(stop)		300
set val(datastop)	250

set ns_ [new Simulator]
set tracefd [open "../trace/test863-25.tr" w]
set recfd [open "../trace/test863-25.rec" w]
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
									-agentTrace OFF \
									-routerTrace OFF \
									-macTrace OFF \
									-movementTrace ON

for {set i 0} {$i < $val(nn)} {incr i} {
	set node_($i) [$ns_ node]
	set agent_($i) [new Agent/DBGAgent [$node_($i) node-addr]]
	$ns_ attach-agent $node_($i) $agent_($i)
	$node_($i) random-motion 0
}

set y 20.0
for {set i 0} {$i < 19} {incr i} {
	$node_($i) set X_ 15.0
	$node_($i) set Y_ $y
	$node_($i) set Z_ 0.0
	set y [expr $y + 10.0]
}

set y 20.0
for {set i 20} {$i < 39} {incr i} {
	$node_($i) set X_ 20.0
	$node_($i) set Y_ $y
	$node_($i) set Z_ 0.0
	set y [expr $y + 10.0]
}

set y 20.0
for {set i 40} {$i < 59} {incr i} {
	$node_($i) set X_ 25.0
	$node_($i) set Y_ $y
	$node_($i) set Z_ 0.0
	set y [expr $y + 10.0]
}

set y 20.0
for {set i 60} {$i < 79} {incr i} {
	$node_($i) set X_ 30.0
	$node_($i) set Y_ $y
	$node_($i) set Z_ 0.0
	set y [expr $y + 10.0]
}

$ns_ at 2.0 "$agent_(0) declare-group 0"
set t 2.0
for {set i 1} {$i < $val(nn)} {incr i} {
	$ns_ at $t "$agent_($i) join-group 0"
	set t [expr $t+0.5]
}

set t2 60
$ns_ at $t2 "senddata"
proc senddata {} {
	global ns_ agent_ val t2
	
	$ns_ at $t2 "$agent_(0) senddata 0 200"
	
	set t2 [expr $t2+0.2]
	if {$t2 < $val(datastop)} {
		$ns_ at $t2 "senddata"
	}
}

proc stop {} {
	global ns_ agent_ val tracefd recfd reccount
	$ns_ flush-trace
	
	set sendcount [$agent_(0) recv-count 0 0]
	set ratio [expr double(0)]
	set lat [expr double(0)]
	puts $recfd "total-send $sendcount"
	for {set i 1} {$i < $val(nn)} {incr i} {
		set recvcount [$agent_($i) recv-count 0 0]
		set r [expr double($recvcount) / double($sendcount)]
		set lt [expr double([$agent_($i) avg-latency 0 0])]
		puts $recfd "$i recv $recvcount $r $lt"
		set ratio [expr $ratio + $r]
		set lat [expr $lat + $lt]
	}
	puts $recfd "average-recv-ratio [expr $ratio / double($val(nn) - 1)]"
	puts $recfd "average-latency [expr $lat / double($val(nn) - 1)]"
	
	close $tracefd
	close $recfd
	exit 0
}

$ns_ at $val(stop).02 "stop"
$ns_ run

