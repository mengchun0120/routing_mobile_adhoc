Agent/DBGAgent set initBroadcastProb_		0.8
Agent/DBGAgent set dropBroadcastProb_		0.1
Agent/DBGAgent set broadcastWait_ 			5
Agent/DBGAgent set broadcastWaitDelay_	0.025
Agent/DBGAgent set broadcastDelay_ 			0.02
Agent/DBGAgent set broadcastJitter_			0.01
Agent/DBGAgent set heartbeatDelay_			0.3
Agent/DBGAgent set broadcastListSize_		400
Agent/DBGAgent set requestWaitShort_		0.1
Agent/DBGAgent set requestWaitLong_			0.3
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
set val(nn)				100
set val(rp)				DBG
set val(x)				1000
set val(y)				1000
set val(stop)			320
set val(datastop)	300
set val(member) 	50

set path /home/mengchun/ns-allinone-2.29/ns-2.29/dbg

set ns_ [new Simulator]
set tracefd [open "$path/trace/test11.tr" w]
set recfd [open "$path/trace/test11.rec" w]
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
									-movementTrace ON

for {set i 0} {$i < $val(nn)} {incr i} {
	set node_($i) [$ns_ node]
	set agent_($i) [new Agent/DBGAgent [$node_($i) node-addr]]
	$ns_ attach-agent $node_($i) $agent_($i)
}

source "$path/scen/scen-n100-s10-x1000-y1000-t320-2"

$ns_ at 2.0 "$agent_(0) declare-group 0"
set t 2.0
for {set i 1} {$i < $val(member)} {incr i} {
	$ns_ at $t "$agent_($i) join-group 0"
	set t [expr $t+0.5]
}

set t 2.0
$ns_ at $t "record"
set connected 1
set t1 $t
set total 0
set reccount 0
set gateways 0
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

proc record {} {
	global ns_ agent_ val recfd t connected t1 total reccount gateways
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
	set c [$agent_(0) gateway-count 0]
	if {$t > 40} {
		incr reccount
		set gateways [expr $gateways + $c]
	}
	puts $recfd "$t gateway-count $c"
	
	puts $recfd "connected=[$agent_(0) connected 0]"
	if {[$agent_(0) connected 0] == 0} {
		if {$connected == 1} {
			set t1 $t
			set connected 0
		}
	} else {
		if {$connected == 0} {
			set total [expr $total + $t - $t1]
			set connected 1
		}
	}
	
	for {set i 0} {$i < $val(nn)} {incr i} {
		if {[$agent_($i) is-gateway 0] == 1} {
			puts $recfd "gateway of $i: [$agent_($i) neighbors 0]"
		}
	}

	set t [expr $t + 0.1]
	if {$t < $val(stop)} {
		$ns_ at $t "record"
	}
}

proc stop {} {
	global ns_ agent_ val tracefd recfd total reccount gateways
	$ns_ flush-trace
	
	puts $recfd "total-disconnected $total"
	puts $recfd "average-gateway-count [expr $gateways / $reccount]"
	
	set sendcount [$agent_(0) recv-count 0 0]
	set ratio [expr double(0)]
	set lat [expr double(0)]
	puts $recfd "total-send $sendcount"
	for {set i 1} {$i < $val(member)} {incr i} {
		set recvcount [$agent_($i) recv-count 0 0]
		set r [expr double($recvcount) / double($sendcount)]
		set lt [expr double([$agent_($i) avg-latency 0 0])]
		puts $recfd "$i recv $recvcount $r $lt"
		set ratio [expr $ratio + $r]
		set lat [expr $lat + $lt]
	}
	puts $recfd "average-recv-ratio [expr $ratio / double($val(member) - 1)]"
	puts $recfd "average-latency [expr $lat / double($val(member) - 1)]"
	
	close $tracefd
	close $recfd
	exit 0
}

$ns_ at $val(stop).02 "stop"
$ns_ run
