# simulation script for 863 research project
# transmission range 250m
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

set val(chan)	 	Channel/WirelessChannel
set val(prop) 		Propagation/TwoRayGround
set val(netif) 		Phy/WirelessPhy
set val(mac) 		Mac/802_11
set val(ifq) 		Queue/DropTail/PriQueue
set val(ll)			LL
set val(ant)		Antenna/OmniAntenna
set val(ifqlen)		100
set val(nn)			30
set val(rp)			DBG
set val(x)			100
set val(y)			500
set val(stop)		120

set ns_ [new Simulator]
set tracefd [open "../trace/test7.tr" w]
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

source "$path/scen/scen-n25-s10"

$ns_ at 2.0 "$agent_(0) declare-group 0"
set t 2.0
for {set i 2} {$i < $val(nn)} {incr i 2} {
	$ns_ at $t "$agent_($i) join-group 0"
	set t [expr $t+0.5]
}

set t 2.0
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
		if {[$agent_($i) is-gateway 0] == 1} {
			puts $recfd "gateway of $i: [$agent_($i) neighbors 0]"
		} else {
			if {[$agent_($i) is-member 0] == 1} {
				puts $recfd "gateway of $i: [$agent_($i) neighbors 0]"
			}
		}
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
