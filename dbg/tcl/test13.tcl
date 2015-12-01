# test to prove the effectiveness and reliability of back-off probabilistic broadcast
# arguments:
#		0		initBroadcastProb_
#		1		speed
#		2		index

set val(prob)	[expr double([lindex $argv 0])]

Agent/DBGAgent set initBroadcastProb_		$val(prob)
Agent/DBGAgent set dropBroadcastProb_		0.1
Agent/DBGAgent set broadcastWait_ 			2
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
set val(speed)		[lindex $argv 1]
set val(index)		[lindex $argv 2]

set path /home/mengchun/ns-allinone-2.29/ns-2.29/dbg

set ns_ [new Simulator]
set tracefd [open "$path/trace/test13-p$val(prob)-s$val(speed)-$val(index).tr" w]
set recfd [open "$path/trace/test13-p$val(prob)-s$val(speed)-$val(index).rec" w]
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
	set prev_($i) 0
}

source "$path/scen/scen-n100-s$val(speed)-x1000-y1000-t320-$val(index)"

set sendcount_ 0
set t 20.0
set k 1
set sum_recvratio 0.0
set sum_srb 0.0

$ns_ at 10.0 "$agent_(0) send-test 0"
$ns_ at $t "step"

proc step {} {
	global ns_ agent_ val prev_ recfd sendcount_ t k sum_recvratio sum_srb
	
	set recvcount 0
	for {set i 1} {$i < $val(nn)} {incr i} {
		set rc [$agent_($i) recv-join-count 0]
		if {$rc > $prev_($i)} {
			incr recvcount
		}
		set prev_($i) $rc
	}
	
	set recvratio [expr double($recvcount) / (double($val(nn)) - 1.0)]
	set sum_recvratio [expr $sum_recvratio + $recvratio]
	
	set sc [$agent_(0) send-count]
	set sc1 [expr $sc - $sendcount_]
	set sendcount_ $sc
	
	set srb [expr 1.0 - double($sc1) / double($recvcount)]
	set sum_srb [expr $sum_srb + $srb]
	
	puts $recfd "$k $t $sc1 $recvcount $recvratio $srb"
	
	set t1 [expr $t + 10.0]
	if {$t1 < $val(stop)} {
		$ns_ at $t "$agent_(0) send-test 0"
		$ns_ at $t1 "step"
		set t $t1
		incr k
	}
}


proc stop {} {
	global ns_ agent_ val tracefd recfd k sum_recvratio sum_srb path
	
	$ns_ flush-trace
	
	set art [expr $sum_recvratio / double($k)]
	set srb [expr $sum_srb / double($k)]
	
	puts $recfd "average-recv-ratio $art"
	puts $recfd "average-srb $srb"
	
	set rd [open "$path/trace/bpb-p$val(prob).txt" a]
	puts $rd "$val(index) $art $srb"
	
	puts "existing test13 $val(prob) $val(index)"
	close $rd
	close $tracefd
	close $recfd
	exit 0
}

$ns_ at $val(stop).02 "stop"
$ns_ run
