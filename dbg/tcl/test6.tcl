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

Phy/WirelessPhy set RXThresh_ 3.65262e-10

set val(chan)	 		Channel/WirelessChannel
set val(prop) 		Propagation/TwoRayGround
set val(netif) 		Phy/WirelessPhy
set val(mac) 			Mac/802_11
set val(ifq) 			Queue/DropTail/PriQueue
set val(ll)				LL
set val(ant)			Antenna/OmniAntenna
set val(ifqlen)		100
set val(nn)				12
set val(rp)				DBG
set val(x)				1000
set val(y)				1000
set val(stop)			50

set path /home/mengchun/ns-allinone-2.29/ns-2.29/dbg

set ns_ [new Simulator]
set tracefd [open "$path/trace/test6.tr" w]
set recfd [open "$path/trace/test6.rec" w]
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
									-routerTrace OFF \
									-macTrace OFF \
									-movementTrace OFF

#-----------------0   1   2   3   4   5   6   7   8   9   10  11
set xlist 	[list 100 200 300 300 400 500 100 200 600 400 200 800]
set ylist 	[list 100 200 200 300 100 200 500 400 100 200 600 100]
set dxlist 	[list 410 110 210 410 310 410 510 610 610 10  410 800]
set dylist  [list 10  510 10  610 10  70  120 210 260 10  610 100]

for {set i 0} {$i < $val(nn)} {incr i} {
	set node_($i) [$ns_ node]
	set agent_($i) [new Agent/DBGAgent [$node_($i) node-addr]]
	$ns_ attach-agent $node_($i) $agent_($i)
	$node_($i) set X_ [lindex $xlist $i]
	$node_($i) set Y_ [lindex $ylist $i]
	$node_($i) set Z_ 0
	$ns_ at 0.0 "$node_($i) setdest [lindex $dxlist $i] [lindex $dylist $i] 0"
}

$ns_ at 1.0 "$agent_(0) declare-group 0"
$ns_ at 2.0 "$agent_(5) join-group 0"
$ns_ at 2.5 "$agent_(4) join-group 0"
$ns_ at 3.0 "$agent_(6) join-group 0"
$ns_ at 3.5 "$agent_(8) join-group 0"
$ns_ at 4.0 "$agent_(10) join-group 0"
$ns_ at 4.5 "$agent_(11) join-group 0"
$ns_ at 5.0 "$node_(9) setdest 400 400 10"
#$ns_ at 15.0 "$node_(9) setdest 400 400 10"

set t 1.0
$ns_ at $t "putdom"

proc putdom {} {
	global ns_ agent_ val recfd t
	puts $recfd "----------------- $t -------------------- "
	set count 0
	puts -nonewline $recfd "not covered:"
	for {set i 0} {$i < $val(nn)} {incr i} {
		if {[$agent_($i) is-member 0] == 1} {
			if {[$agent_($i) covered 0]} {
				incr count
			} else {
				puts -nonewline $recfd " $i"
			}
		}
	}
	flush $recfd
	puts $recfd "\ncovered $count"
	puts $recfd "gateways: [$agent_(0) list-gateway 0]"
	puts $recfd "connected=[$agent_(0) connected 0]"
	for {set i 0} {$i < $val(nn)} {incr i} {
#		if {[$agent_($i) is-gateway 0] == 1} {
			puts $recfd "gateway of $i: [$agent_($i) neighbors 0]"
#		}
	}

	set t [expr $t + 0.1]
	if {$t < $val(stop)} {
		$ns_ at $t "putdom"
	}
}

proc stop {} {
	global ns_ tracefd recfd
	$ns_ flush-trace
	close $tracefd
	close $recfd
	exit 0
}

$ns_ at $val(stop).02 "stop"
$ns_ run
