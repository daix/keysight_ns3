#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("fattree");

//print out the 5-tuple of every packet
#define DEBUG_LABEL 1

//Simulation duration
#define SIMULATION_TIME 10.0

//Configuration: dataRate, delay, errorModel (loss rate when device is receiving pkts).
//E:edge, A:aggregated, C:core; H:host.
#define CA_dataRate "100Mbps"
#define CA_delay "10ms"
#define CA_errorRate 0.00001

#define AE_dataRate "100Mbps"
#define AE_delay "10ms"
#define AE_errorRate 0.00001

#define EH_dataRate "100Mbps"
#define EH_delay "10ms"
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
	if (pkts_in_queue > 0)
		std::cout << pkts_in_queue << std::endl;
	return;
}

void mycallback(uint32_t switch_id, Ptr<const QueueBase> qb, Ptr<const Packet> pkt){
	//	std::cout << "switch id: " << switch_id << std::endl
	//		  << "pkts remaining in queue: " << pkts_in_queue << std::endl
	//		  << "pkts' 5-tuple: " << std::endl;
	//
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

	pkt_dgst.proto = ipv4_protocol;
	Ipv4Address saddr = iph.GetSource(), daddr = iph.GetDestination();
	pkt_dgst.srcAddr = saddr.Get();
	pkt_dgst.dstAddr = daddr.Get();
	
#if DEBUG_LABEL
	std::cout << "switch id: " << switch_id << ", ";
	std::cout << "srcPort: " << pkt_dgst.srcPort << ", dstPort: " << pkt_dgst.dstPort << ", ";
	std::cout << "srcaddr: " << saddr << ", dstaddr: " << daddr << ", ";
	std::cout << "protocol: " << (int)pkt_dgst.proto <<  std::endl;
#endif

	keysight(switch_id, qb->GetNPackets(), pkt_dgst);
}

void mycallback_for_csma(uint32_t switch_id, Ptr<const QueueBase> qb, Ptr<const Packet> pkt){
	//	std::cout << "switch id: " << switch_id << std::endl
	//		  << "pkts remaining in queue: " << pkts_in_queue << std::endl
	//		  << "pkts' 5-tuple: " << std::endl;
	//
	//get #pkts in queue && parse pkt header
	Ptr<Packet> pktcopy = pkt->Copy();
	Pkt_digest pkt_dgst;

	EthernetTrailer trailer;
	pktcopy->RemoveTrailer (trailer);
	    
	EthernetHeader header (false);
	pktcopy->RemoveHeader (header);
	//std::cout << header.GetLengthType() << std::endl;

	if (header.GetLengthType() != 0x0800){//not ipv4
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

	pkt_dgst.proto = ipv4_protocol;
	Ipv4Address saddr = iph.GetSource(), daddr = iph.GetDestination();
	pkt_dgst.srcAddr = saddr.Get();
	pkt_dgst.dstAddr = daddr.Get();
	
#if DEBUG_LABEL
	std::cout << "switch id: " << switch_id << ", ";
	std::cout << "srcPort: " << pkt_dgst.srcPort << ", dstPort: " << pkt_dgst.dstPort << ", ";
	std::cout << "srcaddr: " << saddr << ", dstaddr: " << daddr << ", ";
	std::cout << "protocol: " << (int)pkt_dgst.proto <<  std::endl;
#endif

	keysight(switch_id, qb->GetNPackets(), pkt_dgst);
}
void getQueueSize(uint32_t oldv, uint32_t newv){
	std::cout << "queuesize: " << oldv << ", " << newv << std::endl; 
}

void getQueueSize_bounded(uint32_t switch_id, uint32_t oldv, uint32_t newv){
	std::cout << "switch id: " << switch_id << ", queuesize: " << oldv << ", " << newv << std::endl; 
}
class Fattree {
	public:
		Fattree(uint32_t k = 4);
		~Fattree();

		uint32_t GetHostNumber();
		Ptr<Node> GetHost(uint32_t aggr_idx);
		Ptr<Node> GetHost(uint32_t pod_idx, uint32_t tor_idx, uint32_t idx);
		NodeContainer& GetAllHost();
		Address GetHostSocketAddress(uint32_t aggr_idx, uint32_t port);
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
	p2p_factory.SetDeviceAttribute ("Mtu", UintegerValue(1518));
	em->SetAttribute ("ErrorRate", DoubleValue (AE_errorRate));
	p2p_factory.SetDeviceAttribute("ReceiveErrorModel", PointerValue(em));


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
#if DEBUG_LABEL
					txq->GetObject<QueueBase>()->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(getQueueSize));
#endif
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
#if DEBUG_LABEL
					txq->GetObject<QueueBase>()->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(getQueueSize));
#endif
				}
			}
			core_idx++;
			//std::cout << core_idx << std::endl;
		}
	}

	//Edge <-> Host
	csma_factory.SetChannelAttribute ("DataRate", StringValue (EH_dataRate));
	csma_factory.SetChannelAttribute ("Delay", StringValue(EH_delay));
	csma_factory.SetDeviceAttribute ("Mtu", UintegerValue(1518));
	em->SetAttribute ("ErrorRate", DoubleValue (EH_errorRate));
	csma_factory.SetDeviceAttribute("ReceiveErrorModel", PointerValue(em));

	//TODO:need to switch to p2p device for tracing #pkts_in_queue
	p2p_factory.SetDeviceAttribute("DataRate", StringValue(EH_dataRate));
	p2p_factory.SetChannelAttribute("Delay", StringValue(EH_delay));
	em->SetAttribute ("ErrorRate", DoubleValue (EH_errorRate));
	p2p_factory.SetDeviceAttribute("ReceiveErrorModel", PointerValue(em));
	for (unsigned pod_idx = 0; pod_idx < m_k; ++pod_idx){//pods
		for (unsigned edge_idx = 0; edge_idx < m_k/2; ++edge_idx){//edge switches
			NodeContainer tor;
			tor.Create(m_k/2);
			host_stack.Install(tor);
			hosts.Add(tor);
			for(unsigned host_idx = 0; host_idx < m_k/2; ++host_idx){
				dvc = p2p_factory.Install(pods[pod_idx].Get(edge_idx), tor.Get(host_idx));
				ipv4addr_factory.SetBase(Ipv4Address(ipv4_subnet_generator()), "255.255.255.0");
				ipv4addr_factory.Assign(dvc);
				txq = dvc.Get(0)->GetObject<PointToPointNetDevice>()->GetQueue();
				txq->TraceConnectWithoutContext("Dequeue",MakeBoundCallback(mycallback, dvc.Get(0)->GetNode()->GetId(), txq->GetObject<QueueBase>()));
#if DEBUG_LABEL
				txq->GetObject<QueueBase>()->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(getQueueSize));
#endif
			}

		}
	}
	//	for (unsigned i = 0; i < m_k; ++i){
	//		for (unsigned edge_idx = 0; edge_idx < m_k/2; ++edge_idx){
	//			NodeContainer tor;
	//			tor.Create(m_k/2);
	//			host_stack.Install(tor);
	//			hosts.Add(tor);
	//
	//			tor.Add(pods[i].Get(edge_idx));
	//			dvc = csma_factory.Install(tor);
	//			txq = dvc.Get(m_k/2)->GetObject<CsmaNetDevice>()->GetQueue(); //NOTICE: all hosts connected to the same egde switch share a same csma queue. 
	//			txq->TraceConnectWithoutContext("Enqueue",MakeBoundCallback(mycallback_for_csma, dvc.Get(m_k/2)->GetNode()->GetId(), txq->GetObject<QueueBase>()));
	//#if DEBUG_LABEL
	//			txq->GetObject<QueueBase>()->TraceConnectWithoutContext("PacketsInQueue", MakeBoundCallback(getQueueSize_bounded, dvc.Get(m_k/2)->GetNode()->GetId()));
	//#endif
	//			ipv4addr_factory.SetBase(Ipv4Address(ipv4_subnet_generator()), "255.255.255.0");
	//			ipv4addr_factory.Assign(dvc);
	//		}
	//	}
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
Ptr<Node> Fattree::GetHost(uint32_t aggr_idx){
	aggr_idx %= GetHostNumber();
	return hosts.Get(aggr_idx);	
}

Ptr<Node> Fattree::GetHost(uint32_t pod_idx, uint32_t tor_idx, uint32_t idx){
	//return hosts.Get(idx + m_k/2 * tor_idx + (m_k*m_k/4) *pod_idx)->GetId();
	return hosts.Get(idx + m_k/2 * tor_idx + (m_k*m_k/4) *pod_idx);
}
NodeContainer& Fattree::GetAllHost(){
	return hosts;
}

Ipv4Address getAddr(Ptr<Node> node, uint32_t dev_idx = 1){
	return node->GetObject<Ipv4>()->GetAddress(dev_idx, 0).GetLocal();
}
Address Fattree::GetHostSocketAddress(uint32_t aggr_idx, uint32_t port){
	return Address (InetSocketAddress (getAddr(GetHost(aggr_idx)), port));
}


int main (int argc, char **argv){
	CommandLine cmd;
	uint32_t k = 4;
	cmd.AddValue("k", "number of Ports on a switch", k);
	cmd.Parse(argc, argv);

	Time::SetResolution (Time::NS);
	//LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	//LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
	Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(1));
	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno")); //TcpBic

	Fattree ft(k);
	getAddr(ft.GetHost(0,0,0)).Print(std::cout);

	//for how to understand traffic in DCN, TO BE READ.
	//https://conferences.sigcomm.org/sigcomm/2009/workshops/wren/papers/p65.pdf
	//Random Variable Generator
	//Uniform RV, range [Min, Max], used for change app start time.
	Ptr<UniformRandomVariable> uni_rv = CreateObject<UniformRandomVariable> ();
	uni_rv->SetAttribute("Min", DoubleValue(1));
	uni_rv->SetAttribute("Max", DoubleValue(SIMULATION_TIME));
	//Pareto RV, used for simulating the characteristic of tcp traffic, the expecation of pareto dist is frac{shape*scale}{shape-1}
	Ptr<ParetoRandomVariable> prt_rv = CreateObject<ParetoRandomVariable> ();
	prt_rv->SetAttribute ("Scale", DoubleValue (1000));
	prt_rv->SetAttribute ("Shape", DoubleValue (2));
	//Exponential distribution RV, used for varying the number of flows from a host to another one.
	Ptr<ExponentialRandomVariable> exp_rv = CreateObject<ExponentialRandomVariable> ();
	exp_rv->SetAttribute ("Mean", DoubleValue (100));
	//exp_rv->SetAttribute ("Bound", DoubleValue ());

	uint32_t listenPort = 80;	
	PacketSinkHelper sink_tcp_factory ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), listenPort));
	PacketSinkHelper sink_udp_factory ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), listenPort));

	ApplicationContainer sinker_tcp = sink_tcp_factory.Install(ft.GetAllHost());
	//ApplicationContainer sinker_udp = sink_udp_factory.Install(ft.GetAllHost());
	sinker_tcp.Start(Seconds(0.5));
	//sinker_tcp.Stop(Seconds(SIMULATION_TIME));
	//sinker_udp.Start(Seconds(0.5));
	//sinker_udp.Stop(Seconds(SIMULATION_TIME));

	ApplicationContainer sourcer;

	BulkSendHelper bulk_tcp_factory("ns3::TcpSocketFactory", Address(InetSocketAddress(Ipv4Address("0.0.0.0"), listenPort)));

	//bulk_tcp_factory.SetAttribute ("SendSize", UintegerValue (1458));
	bulk_tcp_factory.SetAttribute ("MaxBytes", UintegerValue (0));//the size of flow should follow a Pareto distribution, 0 means sending without stopping.
	//for (unsigned dstNode = 0; dstNode < ft.GetHostNumber(); ++dstNode){
	//	bulk_tcp_factory.SetAttribute ("Remote", AddressValue (ft.GetHostSocketAddress(dstNode, listenPort)));
	//	sourcer = bulk_tcp_factory.Install(ft.GetAllHost());
	//	sourcer.Start(Seconds(1.0));
	//}
	for (unsigned host_idx = 0; host_idx < ft.GetHostNumber(); ++host_idx){
		for (unsigned host_jdx = 0; host_jdx < ft.GetHostNumber(); ++host_jdx){
			if (host_jdx == host_idx)
				break;
			{//main loop
				uint32_t number_of_flows = exp_rv->GetInteger();
				for (unsigned nof = 0; nof < number_of_flows; ++nof){
					bulk_tcp_factory.SetAttribute ("Remote", AddressValue (ft.GetHostSocketAddress(host_jdx, listenPort)));
					bulk_tcp_factory.SetAttribute("MaxBytes", UintegerValue(prt_rv->GetInteger()));
					sourcer=bulk_tcp_factory.Install(ft.GetHost(host_idx));
					sourcer.Start(Seconds(uni_rv->GetValue()));
				}
			}
		}
	}
	//std::cout << "installing flows finished." << std::endl;

	//https://codereview.appspot.com/318880043/ for advanced onoff app
	//OnOffHelper onoff_tcp_factory ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address ("0.0.0.0"), listenPort)));
	//onoff_tcp_factory.SetConstantRate (DataRate ("10Mbps"));
	//onoff_tcp_factory.SetAttribute("PacketSize", UintegerValue (2000));

	//std::cout << ft.GetHostNumber() << std::endl;
	//for (unsigned i = 0; i < ft.GetHostNumber(); ++i){
	//	onoff_tcp_factory.SetAttribute ("Remote", AddressValue (ft.GetHostSocketAddress(i, listenPort)));

	//	sourcer = onoff_tcp_factory.Install(ft.GetAllHost());
	//	sourcer.Start(Seconds(1.0));
	//	//sourcer.Stop(Seconds(2.0));
	//}
	std::cout << "installing flows finished." << std::endl;


	//OnOffHelper onoff_udp_factory ("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address ("10.0.0.1"), listenPort)));
	//onoff_udp_factory.SetConstantRate (DataRate (App_dataRate));
	//onoff_udp_factory.SetAttribute("PacketSize", UintegerValue (2000));

	//the switchs' ids range from [0, #switch)
	//for (unsigned pod_idx = 0; pod_idx < k; ++pod_idx){
	//	for (unsigned tor_idx = 0; tor_idx < k/2; ++tor_idx){
	//		for (unsigned idx = 0; idx < k/2; ++idx){
	//			std::cout << ft.GetHost(pod_idx, tor_idx, idx)->GetId() << std::endl;
	//		}
	//	}
	//}

	////ft.getK();
	////std::cout << ft.ipv4_subnet_generator() << std::endl;

	////std::cout << ft.GetHostNumber() << std::endl;
	////std::cout << ft.GetHost(0, 0, 0) << std::endl;
	////std::cout << ft.GetHost(0, 0, 1) << std::endl;
	////std::cout << ft.GetHost(0, 1, 0) << std::endl;
	////std::cout << ft.GetHost(0, 1, 0) << std::endl;
	////std::cout << ft.GetHost(2, 1, 0) << std::endl;
	////std::cout << ft.GetHost(2, 1, 1) << std::endl;
	////std::cout << ft.GetHost(2, 1, 1)->GetId() << std::endl;

	////NS_LOG_UNCOND (getAddr(ft.GetHost(3,0,0)));

	//UdpEchoServerHelper echoServer (9);
	//ApplicationContainer serverApps = echoServer.Install (ft.GetHost(0,0,0));
	//serverApps.Start (Seconds (1.0));
	//serverApps.Stop (Seconds (10.0));

	//UdpEchoClientHelper echoClient (getAddr(ft.GetHost(0,0,0)), 9);
	//echoClient.SetAttribute ("MaxPackets", UintegerValue (5));
	//echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	//echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

	////ApplicationContainer clientApps = echoClient.Install (ft.GetHost(0,0,1));
	//////ApplicationContainer clientApps = echoClient.Install (ft.GetHost(0,1,0));
	//NodeContainer clients;
	////clients.Add(ft.GetHost(0,0,1));
	////clients.Add(ft.GetHost(0,1,0));
	//clients.Add(ft.GetHost(2,1,0));
	////clients.Add(ft.GetHost(0,1,0));
	////clients.Add(ft.GetHost(0,1,1));
	////clients.Add(ft.GetHost(0,1,1));
	////clients.Add(ft.GetHost(0,1,1));
	//////ApplicationContainer clientApps = echoClient.Install (ft.GetHost(2,1,0));
	//ApplicationContainer clientApps = echoClient.Install (clients);
	//clientApps.Start (Seconds (2.0));
	//clientApps.Stop (Seconds (10.0));

	//PacketSinkHelper sink_factory ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 80));
	//ApplicationContainer sinker = sink_factory.Install(ft.GetHost(0,0,0));
	//sinker.Start(Seconds(1.0));
	//sinker.Stop(Seconds(10.0));

	//BulkSendHelper source_factory ("ns3::TcpSocketFactory", Address(InetSocketAddress(getAddr(ft.GetHost(0,0,0)), 80)));
	//NodeContainer sender;
	//sender.Add(ft.GetHost(2,1,1));
	////sender.Add(ft.GetHost(2,0,1));
	////sender.Add(ft.GetHost(1,1,1));
	//sender.Add(ft.GetHost(2,1,1));
	////sender.Add(ft.GetHost(2,1,1));
	////sender.Add(ft.GetHost(2,0,1));
	////sender.Add(ft.GetHost(1,1,1));
	////sender.Add(ft.GetHost(2,0,1));
	////sender.Add(ft.GetHost(1,1,1));
	//ApplicationContainer sourcer = source_factory.Install(sender);
	//sourcer.Start(Seconds(1.5));
	//sourcer.Stop(Seconds(2.5));

	Simulator::Stop(Seconds(SIMULATION_TIME));
	Simulator::Run();
	Simulator::Destroy();

	return 0;
}
