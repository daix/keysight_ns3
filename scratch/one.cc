#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("one");

void mycallback (uint32_t id, Ptr<const QueueBase> qb, Ptr<const Packet> pkt){
	std::cout << "a: " << id << ", n: " << qb->GetNPackets() <<  ", catch a enqueue" << std::endl;
}
void getQueueSize(uint32_t oldv, uint32_t newv){
	std::cout << "queuesize: " << oldv << ", " << newv << std::endl; 
}

int main (int argc, char **argv){
	CommandLine cmd;
	uint32_t nPorts_of_switch = 4;
	cmd.AddValue("K", "number of Ports on a switch", nPorts_of_switch);
	cmd.Parse(argc, argv);

	//std::cout << nPorts_of_switch << std::endl;
	
	NodeContainer nc;
	nc.Create(2);

	PointToPointHelper p2pfcty;
	p2pfcty.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
	p2pfcty.SetChannelAttribute("Delay", StringValue("100ms"));
	
	NetDeviceContainer dc;
	dc = p2pfcty.Install(nc);
	Ptr<Queue<Packet>> txq = dc.Get(0)->GetObject<PointToPointNetDevice>()->GetQueue();
	if (txq){
		std::cout << "found txqueue" << std::endl;
		txq->TraceConnectWithoutContext("Enqueue",MakeBoundCallback(mycallback, nc.Get(0)->GetId(), txq->GetObject<QueueBase>()));
		txq->GetObject<QueueBase>()->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(getQueueSize));
	}else{
		std::cout << "not found" << std::endl;
	}
	InternetStackHelper isfcty;
	isfcty.Install(nc);

	Ipv4AddressHelper ipv4fcty;
	ipv4fcty.SetBase("10.0.0.0", "255.255.255.0");

	Ipv4InterfaceContainer ifs = ipv4fcty.Assign(dc);
	
	//UdpEchoServerHelper echoServer(100);

	//ApplicationContainer serverApps = echoServer.Install(nc.Get(1));
	//serverApps.Start(Seconds(1.0));
	//serverApps.Stop(Seconds(10.0));

	//UdpEchoClientHelper echoClient(ifs.GetAddress(1), 9);
	//echoClient.SetAttribute ("MaxPackets", UintegerValue(100));
	//echoClient.SetAttribute ("Interval", TimeValue (NanoSeconds (1)));
	//echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

	//ApplicationContainer clientApps = echoClient.Install(nc.Get(0));
	//clientApps.Start(Seconds(2.0));
	//clientApps.Stop(Seconds(10.0));

	//int tcpSegmentSize = 1024; 
	//uint32_t maxBytes = 1000000; // 0 means "unlimited"
	//Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcpSegmentSize));
	//Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0)); 

	Address sinkAaddr(InetSocketAddress (ifs.GetAddress(1), 80)); 
	//PacketSinkHelper sink("ns3::TcpSockerFactory", sinkAaddr);
  	PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 80));
	ApplicationContainer sinkapp = sink.Install(nc.Get(1));
	sinkapp.Start(Seconds(1.0));
	sinkapp.Stop(Seconds(10.0));

	BulkSendHelper sourceAhelper ("ns3::TcpSocketFactory", sinkAaddr);
	//sourceAhelper.SetAttribute ("MaxBytes", UintegerValue (maxBytes)); 
	//sourceAhelper.SetAttribute ("SendSize", UintegerValue (tcpSegmentSize)); 
	ApplicationContainer sourceapp = sourceAhelper.Install(nc.Get(0));
	sourceapp.Start(Seconds(1.5));
	sourceapp.Stop(Seconds(2.5));

	Simulator::Stop(Seconds(10.0));

  	AsciiTraceHelper ascii;
	p2pfcty.EnableAscii(ascii.CreateFileStream("log.txt"), dc.Get(0));

	Simulator::Run();
	Simulator::Destroy();

	return 0;
}
