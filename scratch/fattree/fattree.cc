#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("fattree");

//Configuration: dataRate, delay, errorModel (loss rate when device is receiving pkts).
//E:edge, A:aggregated, C:core; H:host.
#define CA_dataRate "100Mbps"
#define CA_delay "100ms"
#define CA_errorRate 0.00001

#define AE_dataRate "100Mbps"
#define AE_delay "100ms"
#define AE_errorRate 0.00001

#define EH_dataRate "100Mbps"
#define EH_delay "100ms"
#define EH_errorRate 0.00001

class Pkt_digest {
	public:
		//host-order 32-bit ip address
		uint32_t srcAddr;
		uint32_t dstAddr;
		//port
		uint16_t srcPort;
		uint16_t dstPort;
		uint8_t proto;	
};

void keysight(uint32_t switch_id, uint32_t pkts_in_queue, Pkt_digest& pkt_dgst){
	//std::cout << "switch id: " << switch_id << ",  pkts in queue: " << pkts_in_queue << std::endl;  
	return;
}

void mycallback(uint32_t switch_id, Ptr<const QueueBase> qb, Ptr<const Packet> pkt){
	//	std::cout << "switch id: " << switch_id << std::endl
	//		  << "pkts remaining in queue: " << pkts_in_queue << std::endl
	//		  << "pkts' 5-tuple: " << std::endl;
	//get #pkts in queue && parse pkt header

	Ptr<Packet> pktcopy = pkt->Copy();
	Pkt_digest pkt_dgst;

	PppHeader ppp;
	pktcopy->RemoveHeader(ppp);
	//std::cout << ppp.GetProtocol() << ", ";
	if (ppp.GetProtocol() != 0x0021 ){//not ipv4
		return;
	}
	Ipv4Header iph;
	pktcopy->RemoveHeader(iph);
	uint8_t ipv4_protocol = iph.GetProtocol();
	if (ipv4_protocol == 6){//tcp
		TcpHeader tcph;
		pktcopy->PeekHeader(tcph);
		pkt_dgst.srcPort = tcph.GetSourcePort();
		pkt_dgst.dstPort = tcph.GetDestinationPort ();
	}else if (ipv4_protocol == 17){//udp
		UdpHeader udph;
		pktcopy->PeekHeader(udph);
		pkt_dgst.srcPort = udph.GetSourcePort();
		pkt_dgst.dstPort = udph.GetDestinationPort ();
	}else{
		return;
	}
	//std::cout << "srcPort: " << pkt_dgst.srcPort << ", dstPort: " << pkt_dgst.dstPort << ", ";

	pkt_dgst.proto = ipv4_protocol;
	Ipv4Address saddr = iph.GetSource(), daddr = iph.GetDestination();
	pkt_dgst.srcAddr = saddr.Get();
	pkt_dgst.dstAddr = daddr.Get();
	//std::cout << "srcaddr: " << saddr << ", dstaddr: " << daddr << ", ";
	//std::cout << "protocol: " << (int)pkt_dgst.proto <<  std::endl;


	keysight(switch_id, qb->GetNPackets(), pkt_dgst);
}

void getQueueSize(uint32_t oldv, uint32_t newv){
	std::cout << "queuesize: " << oldv << ", " << newv << std::endl; 
}

class Fattree {
	public:
		Fattree(uint32_t k = 4);
		~Fattree();

		uint32_t GetHostNumber();
		Ptr<Node> GetHost(uint32_t pod_idx, uint32_t tor_idx, uint32_t idx);
		void getK();
	private:
		void init();
		void setup();
		uint32_t ipv4_subnet_generator();

		uint32_t m_k; 
		NodeContainer *pods;
		NodeContainer hosts, cores;
};

void Fattree::getK(){
	std::cout << m_k << std::endl;
}

uint32_t Fattree::ipv4_subnet_generator(){
	static uint32_t a = 10, b = 1, c = 0, d = 0;
	//a.b.c.d
	c++;
	if (c==255){
		c = 1;
		b++;
	}
	if (b==255){
		b = 1;
		a++;
	}
	//std::cout << a << "." << b << "." << c << "." << d << std::endl;
	return (a * (1 << 24)) + (b * (1 << 16)) + (c * (1 << 8)) + d;
}
void Fattree::init(){
	pods = new NodeContainer[m_k];
	//cores = new NodeContainer;
	//hosts = new NodeContainer;

	setup();
}
void Fattree::setup(){
	PointToPointHelper p2p_factory;
	CsmaHelper csma_factory;
	Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();

	InternetStackHelper switch_stack;
	//Ipv4HashRoutingHelper hashHelper;
	//switch_stack.SetRoutingHelper(hashHelper);

	InternetStackHelper host_stack;

	Ipv4AddressHelper ipv4addr_factory;

	NetDeviceContainer dvc;

	//callback sourcer: TxQueue
	Ptr<Queue<Packet>> txq;

	//Aggr <-> Edge
	p2p_factory.SetDeviceAttribute("DataRate", StringValue(AE_dataRate));
	p2p_factory.SetChannelAttribute("Delay", StringValue(AE_delay));
	em->SetAttribute ("ErrorRate", DoubleValue (AE_errorRate));
	p2p_factory.SetDeviceAttribute("ReceiveErrorModel", PointerValue(em));

	//Edge <-> Host
	csma_factory.SetChannelAttribute ("DataRate", StringValue (EH_dataRate));
	csma_factory.SetChannelAttribute ("Delay", StringValue(EH_delay));
	csma_factory.SetDeviceAttribute ("Mtu", UintegerValue(1518));
	em->SetAttribute ("ErrorRate", DoubleValue (EH_errorRate));
	csma_factory.SetDeviceAttribute("ReceiveErrorModel", PointerValue(em));

	for (unsigned i = 0; i < m_k; ++i){

		pods[i].Create(m_k);
		switch_stack.Install(pods[i]);

		for (unsigned aggr_idx = m_k/2; aggr_idx < m_k; ++aggr_idx){
			for (unsigned edge_idx = 0; edge_idx < m_k/2; ++edge_idx){
				dvc = p2p_factory.Install(pods[i].Get(aggr_idx), pods[i].Get(edge_idx));	
				ipv4addr_factory.SetBase(Ipv4Address(ipv4_subnet_generator()), "255.255.255.0");
				ipv4addr_factory.Assign(dvc);
				for (unsigned pair = 0; pair < 2; ++pair){
					txq = dvc.Get(pair)->GetObject<PointToPointNetDevice>()->GetQueue();
					txq->TraceConnectWithoutContext("Dequeue",MakeBoundCallback(mycallback, dvc.Get(pair)->GetNode()->GetId(), txq->GetObject<QueueBase>()));
					//txq->GetObject<QueueBase>()->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(getQueueSize));
				}
			}
		}

		//for (unsigned edge_idx = 0; edge_idx < m_k/2; ++edge_idx){
		//	NodeContainer tor;
		//	tor.Create(m_k/2);
		//	host_stack.Install(tor);
		//	hosts.Add(tor);

		//	tor.Add(pods[i].Get(edge_idx));
		//	dvc = csma_factory.Install(tor);
		//	ipv4addr_factory.SetBase(Ipv4Address(ipv4_subnet_generator()), "255.255.255.0");
		//	ipv4addr_factory.Assign(dvc);
		//}
	}

	//cores
	cores.Create(m_k * m_k / 4);
	switch_stack.Install(cores);

	p2p_factory.SetDeviceAttribute("DataRate", StringValue(CA_dataRate));
	p2p_factory.SetChannelAttribute("Delay", StringValue(CA_delay));
	em->SetAttribute ("ErrorRate", DoubleValue (CA_errorRate));
	p2p_factory.SetDeviceAttribute("ReceiveErrorModel", PointerValue(em));

	unsigned core_idx = 0;
	for (unsigned aggr_idx = m_k/2; aggr_idx < m_k; ++aggr_idx){
		for (unsigned i = 0; i < m_k/2; ++i){
			for (unsigned pod_idx = 0; pod_idx < m_k; ++pod_idx){
				dvc = p2p_factory.Install(pods[pod_idx].Get(aggr_idx), cores.Get(core_idx));	
				ipv4addr_factory.SetBase(Ipv4Address(ipv4_subnet_generator()), "255.255.255.0");
				ipv4addr_factory.Assign(dvc);
				for (unsigned pair = 0; pair < 2; ++pair){
					txq = dvc.Get(pair)->GetObject<PointToPointNetDevice>()->GetQueue();
					txq->TraceConnectWithoutContext("Dequeue",MakeBoundCallback(mycallback, dvc.Get(pair)->GetNode()->GetId(), txq->GetObject<QueueBase>()));
					//txq->GetObject<QueueBase>()->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(getQueueSize));
				}
			}
			core_idx++;
			//std::cout << core_idx << std::endl;
		}
	}

	for (unsigned i = 0; i < m_k; ++i){
		for (unsigned edge_idx = 0; edge_idx < m_k/2; ++edge_idx){
			NodeContainer tor;
			tor.Create(m_k/2);
			host_stack.Install(tor);
			hosts.Add(tor);

			tor.Add(pods[i].Get(edge_idx));
			dvc = csma_factory.Install(tor);
			ipv4addr_factory.SetBase(Ipv4Address(ipv4_subnet_generator()), "255.255.255.0");
			ipv4addr_factory.Assign(dvc);
		}
	}
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}
Fattree::Fattree(uint32_t k)
	:m_k(k)
{
	init();
}
Fattree::~Fattree(){
	delete []pods;
	//delete cores;
	//delete hosts;
}

uint32_t Fattree::GetHostNumber(){
	//for (unsigned i = 0; i < hosts.GetN(); ++i)
	//	std::cout << hosts.Get(i)->GetId() << std::endl;
	return hosts.GetN();
}

Ptr<Node> Fattree::GetHost(uint32_t pod_idx, uint32_t tor_idx, uint32_t idx){
	//return hosts.Get(idx + m_k/2 * tor_idx + (m_k*m_k/4) *pod_idx)->GetId();
	return hosts.Get(idx + m_k/2 * tor_idx + (m_k*m_k/4) *pod_idx);
}

Ipv4Address getAddr(Ptr<Node> node, uint32_t dev_idx = 1){
	return node->GetObject<Ipv4>()->GetAddress(dev_idx, 0).GetLocal();
}

int main (int argc, char **argv){
	CommandLine cmd;
	uint32_t k = 4;
	cmd.AddValue("k", "number of Ports on a switch", k);
	cmd.Parse(argc, argv);

	Time::SetResolution (Time::NS);
	//LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	//LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
	Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(1));

	Fattree ft(k);

	for (unsigned pod_idx = 0; pod_idx < k; ++pod_idx){
		for (unsigned tor_idx = 0; tor_idx < k/2; ++tor_idx){
			for (unsigned idx = 0; idx < k/2; ++idx){
				std::cout << ft.GetHost(pod_idx, tor_idx, idx)->GetId() << std::endl;
			}
		}
	}

	//ft.getK();
	//std::cout << ft.ipv4_subnet_generator() << std::endl;

	//std::cout << ft.GetHostNumber() << std::endl;
	//std::cout << ft.GetHost(0, 0, 0) << std::endl;
	//std::cout << ft.GetHost(0, 0, 1) << std::endl;
	//std::cout << ft.GetHost(0, 1, 0) << std::endl;
	//std::cout << ft.GetHost(0, 1, 0) << std::endl;
	//std::cout << ft.GetHost(2, 1, 0) << std::endl;
	//std::cout << ft.GetHost(2, 1, 1) << std::endl;
	//std::cout << ft.GetHost(2, 1, 1)->GetId() << std::endl;

	//NS_LOG_UNCOND (getAddr(ft.GetHost(3,0,0)));


	UdpEchoServerHelper echoServer (9);
	ApplicationContainer serverApps = echoServer.Install (ft.GetHost(0,0,0));
	serverApps.Start (Seconds (1.0));
	serverApps.Stop (Seconds (10.0));

	UdpEchoClientHelper echoClient (getAddr(ft.GetHost(0,0,0)), 9);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (5));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

	//ApplicationContainer clientApps = echoClient.Install (ft.GetHost(0,0,1));
	////ApplicationContainer clientApps = echoClient.Install (ft.GetHost(0,1,0));
	NodeContainer clients;
	//clients.Add(ft.GetHost(0,0,1));
	//clients.Add(ft.GetHost(0,1,0));
	clients.Add(ft.GetHost(2,1,0));
	//clients.Add(ft.GetHost(0,1,0));
	//clients.Add(ft.GetHost(0,1,1));
	//clients.Add(ft.GetHost(0,1,1));
	//clients.Add(ft.GetHost(0,1,1));
	////ApplicationContainer clientApps = echoClient.Install (ft.GetHost(2,1,0));
	ApplicationContainer clientApps = echoClient.Install (clients);
	clientApps.Start (Seconds (2.0));
	clientApps.Stop (Seconds (10.0));

	PacketSinkHelper sink_factory ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 80));
	ApplicationContainer sinker = sink_factory.Install(ft.GetHost(0,0,0));
	sinker.Start(Seconds(1.0));
	sinker.Stop(Seconds(10.0));

	BulkSendHelper source_factory ("ns3::TcpSocketFactory", Address(InetSocketAddress(getAddr(ft.GetHost(0,0,0)), 80)));
	NodeContainer sender;
	sender.Add(ft.GetHost(2,1,1));
	//sender.Add(ft.GetHost(2,0,1));
	//sender.Add(ft.GetHost(1,1,1));
	//sender.Add(ft.GetHost(2,1,1));
	//sender.Add(ft.GetHost(2,1,1));
	//sender.Add(ft.GetHost(2,0,1));
	//sender.Add(ft.GetHost(1,1,1));
	//sender.Add(ft.GetHost(2,0,1));
	//sender.Add(ft.GetHost(1,1,1));
	ApplicationContainer sourcer = source_factory.Install(sender);
	sourcer.Start(Seconds(1.5));
	sourcer.Stop(Seconds(2.5));

	Simulator::Stop(Seconds(10));
	Simulator::Run();
	Simulator::Destroy();

	return 0;
}
