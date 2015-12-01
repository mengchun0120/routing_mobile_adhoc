Agent/DBGAgent set initBroadcastProb_		0.8
Agent/DBGAgent set dropBroadcastProb_		0.1
Agent/DBGAgent set broadcastWait_ 			5
Agent/DBGAgent set broadcastWaitDelay_		0.025
Agent/DBGAgent set broadcastDelay_ 			0.02
Agent/DBGAgent set broadcastJitter_			0.01
Agent/DBGAgent set heartbeatDelay_			0.1
Agent/DBGAgent set broadcastListSize_		400
Agent/DBGAgent set requestWaitShort_		0.1
Agent/DBGAgent set requestWaitLong_			2
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
set val(stop)		70

set path /home/mengchun/ns-allinone-2.29/ns-2.29/dbg

set ns_ [new Simulator]
set tracefd [open "$path/trace/test3.tr" w]
set namtrace [open "$path/trace/test3.nam" w]
$ns_ trace-all $tracefd
$ns_ namtrace-all-wireless $namtrace $val(x) $val(y)

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

set xrng [new RNG]
$xrng seed 0
set xrand [new RandomVariable/Uniform]
$xrand set min_ 0
$xrand set max_ $val(x)
$xrand use-rng $xrng

set yrng [new RNG]
$yrng seed 0
set yrand [new RandomVariable/Uniform]
$yrand set min_ 0
$yrand set max_ $val(y)
$yrand use-rng $yrng

for {set i 0} {$i < $val(nn)} {incr i} {
	set node_($i) [$ns_ node]
	set agent_($i) [new Agent/DBGAgent [$node_($i) node-addr]]
	$ns_ attach-agent $node_($i) $agent_($i)
	$node_($i) random-motion 0
	$node_($i) set X_ [$xrand value]
	$node_($i) set Y_ [$yrand value]
	$node_($i) set Z_ 0
	$ns_ at 0.0 "$node_($i) setdest 100 100 0"
}

$ns_ at 10.0 "$agent_(0) declare-group 0"
set t 10.0
for {set i 2} {$i < $val(nn)} {incr i 2} {
	$ns_ at $t "$agent_($i) join-group 0"
	set t [expr $t+1]
}

proc putcovered {} {
	global agent_ val
	set count 0
	for {set i 0} {$i < $val(nn)} {incr i 2} {
		if {[$agent_($i) covered 0]} {
			incr count
		} else {
			puts "$i not covered"
		}
	}
	puts "covered $count"
}

proc putdom {} {
	global agent_ val recfd
	for {set i 0} {$i < $val(nn)} {incr i} {
		if {[$agent_($i) is-dom 0]} {
			puts $i
		}
	}
	for {set i 0} {$i < $val(nn)} {incr i} {
		$agent_($i) print-dom 0
	}
}


proc stop {} {
	global ns_ tracefd namtrace
	$ns_ flush-trace
	close $tracefd
	close $namtrace
	putcovered
	putdom
	exit 0
}

$ns_ at $val(stop).02 "stop"
$ns_ run
