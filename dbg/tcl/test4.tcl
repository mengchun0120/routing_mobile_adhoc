Agent/DBGAgent set initBroadcastProb_		0.8
Agent/DBGAgent set dropBroadcastProb_		0.1
Agent/DBGAgent set broadcastWait_ 			5
Agent/DBGAgent set broadcastWaitDelay_	0.025
Agent/DBGAgent set broadcastDelay_ 			0.02
Agent/DBGAgent set broadcastJitter_			0.01
Agent/DBGAgent set heartbeatDelay_			0.1
Agent/DBGAgent set broadcastListSize_		400
Agent/DBGAgent set requestWaitShort_		0.1
Agent/DBGAgent set requestWaitLong_			2
Agent/DBGAgent set checkDelay_				0.05
Agent/DBGAgent set leaveThreshold_			0.5

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
set val(nn)			11
set val(rp)			DBG
set val(x)			1000
set val(y)			1000
set val(stop)		20

set path /home/mengchun/ns-allinone-2.29/ns-2.29/dbg

set ns_ [new Simulator]
set tracefd [open "$path/trace/test4.tr" w]
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
#-----------------0   1   2   3   4   5   6   7   8   9   10
set xlist 	[list 10  110 210 410 310 170 270 410 160 10  410]
set ylist 	[list 10  110 210 410 310 70  120 210 260 310 110]
set dxlist 	[list 410 110 210 410 310 410 510 610 610 10  410]
set dylist  [list 10  510 10  610 10  70  120 210 260 10  610]

for {set i 0} {$i < $val(nn)} {incr i} {
	set node_($i) [$ns_ node]
	set agent_($i) [new Agent/DBGAgent [$node_($i) node-addr]]
	$ns_ attach-agent $node_($i) $agent_($i)
	$node_($i) set X_ [lindex $xlist $i]
	$node_($i) set Y_ [lindex $ylist $i]
	$node_($i) set Z_ 0
	$ns_ at 0.0 "$node_($i) setdest [lindex $dxlist $i] [lindex $dylist $i] 0"
}

$ns_ at 1.0 "$agent_(3) declare-group 0"
$ns_ at 1.5 "$agent_(0) join-group 0"
$ns_ at 2.0 "$agent_(2) join-group 0"
$ns_ at 2.5 "$agent_(4) join-group 0"
$ns_ at 3.0 "$agent_(8) join-group 0"
$ns_ at 3.5 "$agent_(9) join-group 0"
$ns_ at 3.5 "$agent_(7) join-group 0"

proc putdom {} {
	global agent_ val
	puts -nonewline "gateways:"
	for {set i 0} {$i < $val(nn)} {incr i} {
		if {[$agent_($i) is-gateway 0]} {
			puts -nonewline " $i"
		}
	}
	puts -nonewline "\n"
	flush stdout
	for {set i 0} {$i < $val(nn)} {incr i} {
		puts "neighbors of $i: [$agent_($i) neighbors 0]"
	}
}

proc stop {} {
	global ns_ tracefd
	$ns_ flush-trace
	close $tracefd
	putdom
	exit 0
}

$ns_ at $val(stop).02 "stop"
$ns_ run
