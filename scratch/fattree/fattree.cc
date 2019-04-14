#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include "ns3/mpi-interface.h"
#ifdef NS3_MPI
#include <mpi.h>
#endif

#include <iostream>
#include <vector>
#include "pkt_digest.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("fattree");

#define DEBUG_LABEL 0

#define SIMULATION_TIME 10.0

#define DATA_RATE "1Gbps"
#define DELAY "100us"
#define ERROR_RATE 0.00001
#define QUEUE_SIZE_IN_PKTS "1000p"

struct {
#ifdef NS3_MPI
	uint32_t systemId;
	uint32_t systemCount;
#endif
	std::vector<std::ofstream> logging;
} Context;


void keysight(uint32_t switch_id, uint32_t dev_id, uint32_t pkts_in_queue, Pkt_digest& pkt_dgst){

	Context.logging[switch_id] << Simulator::Now().GetNanoSeconds() << " " << switch_id << " " << dev_id << " " << pkts_in_queue << " " 
		<< pkt_dgst.srcAddr << " " << pkt_dgst.dstAddr << " " << pkt_dgst.srcPort << " " << pkt_dgst.dstPort << " " 
		<< (uint32_t)pkt_dgst.proto << std::endl;
	//main logic here
	//bfq or other statistics
}

void enq_event(uint32_t switch_id, uint32_t dev_id, Ptr<const QueueBase> qb, Ptr<const Packet> pkt){

	//parse the pkt header
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
	std::cout << "device id: " << dev_id << ", ";
	std::cout << "srcPort: " << pkt_dgst.srcPort << ", dstPort: " << pkt_dgst.dstPort << ", ";
	std::cout << "srcaddr: " << saddr << ", dstaddr: " << daddr << ", ";
	std::cout << "protocol: " << (int)pkt_dgst.proto <<  std::endl;
#endif
	keysight(switch_id, dev_id-1, qb->GetNPackets(), pkt_dgst);
}

void qsize_event(uint32_t switch_id, uint32_t oldv, uint32_t newv){
	std::cout << "switch id: " << switch_id << ", queuesize: " << oldv << ", " << newv << std::endl; 
}

class Fattree {
	public:
		Fattree(uint32_t);
		~Fattree();

		uint32_t GetHostNumber();
		uint32_t GetSwitchNumber();
		uint32_t GetHostNumberInPod();
		Ptr<Node> GetHost(uint32_t);
		Ptr<Node> GetHost(uint32_t pod_idx, uint32_t idx);
		Ptr<Node> GetHost(uint32_t pod_idx, uint32_t tor_idx, uint32_t idx);
		Ipv4Address GetAddress(uint32_t);
		Ipv4Address GetAddress(uint32_t pod_idx, uint32_t tor_idx, uint32_t idx);
	private:
		bool check_logic_processor();
		void setup();
		void enable_logging();

		uint32_t m_k; 
		uint32_t m_nSwitch, m_nHip;

		NodeContainer *pods;
		NodeContainer hosts, cores;

		Ipv4InterfaceContainer hosts_addr;
};

Fattree::Fattree(uint32_t k = 4):m_k(k){
	m_nSwitch = 5 * m_k * m_k / 4;
	m_nHip = m_k * m_k / 4;
	check_logic_processor();
	pods = new NodeContainer[m_k];
	enable_logging();
	setup();
}
Fattree::~Fattree(){
	delete []pods;
	for (uint32_t i = 0; i < 5 * m_k * m_k / 4; ++i)
		Context.logging[i].close();
}

bool Fattree::check_logic_processor(){
#ifdef NS3_MPI
	if (Context.systemCount < m_k + 1){
		std::cout << "This simulation requires " << m_k+1 << " and only " <<  Context.systemCount <<" logical processors." << std::endl;
		return false;
	}
#endif
	return true;
}

void Fattree::enable_logging(){
	std::string filepath = "KS_DATA/";
	for (uint32_t i = 0; i < 5 * m_k * m_k / 4; ++i)
		Context.logging.push_back(std::move(std::ofstream(filepath+std::to_string(i))));
}
void Fattree::setup(){

	PointToPointHelper p2p_factory;

	//p2p settings
	p2p_factory.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (QUEUE_SIZE_IN_PKTS));
	p2p_factory.SetDeviceAttribute("DataRate", StringValue(DATA_RATE));
	p2p_factory.SetChannelAttribute("Delay", StringValue(DELAY));
	p2p_factory.SetDeviceAttribute ("Mtu", UintegerValue(1518));
	Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
	em->SetAttribute ("ErrorRate", DoubleValue (ERROR_RATE));
	p2p_factory.SetDeviceAttribute("ReceiveErrorModel", PointerValue(em));

	InternetStackHelper stack;
	//Ipv4GlobalRoutingHelper routingHelper;
	//stack.SetRoutingHelper(routingHelper);

	Ipv4AddressHelper ipv4addr_factory;
	ipv4addr_factory.SetBase("10.0.0.0", "255.255.255.0");

	NetDeviceContainer tmpdvc;
	Ptr<Queue<Packet>> txq;//Trace Source

	//pod switches
	for (uint32_t i = 0; i < m_k; ++i){
#ifdef NS3_MPI
		pods[i].Create(m_k, i);
#else
		pods[i].Create(m_k);
#endif
		stack.Install(pods[i]);
		for (uint32_t agg_idx = m_k/2; agg_idx < m_k; ++agg_idx){
			for (uint32_t edge_idx = 0; edge_idx < m_k/2; ++edge_idx){
				tmpdvc = p2p_factory.Install(pods[i].Get(agg_idx), pods[i].Get(edge_idx));
				ipv4addr_factory.NewNetwork();
				ipv4addr_factory.Assign(tmpdvc);
				//TODO:Trace
				//if ( Context.systemId == i ) {
				for (uint32_t pair = 0; pair < 2; ++pair){
					uint32_t switch_id = tmpdvc.Get(pair)->GetNode()->GetId();
					txq = tmpdvc.Get(pair)->GetObject<PointToPointNetDevice>()->GetQueue();
					txq->TraceConnectWithoutContext("Enqueue",MakeBoundCallback(enq_event, switch_id, tmpdvc.Get(pair)->GetIfIndex(), txq->GetObject<QueueBase>()));
#if DEBUG_LABEL
					txq->GetObject<QueueBase>()->TraceConnectWithoutContext("PacketsInQueue", MakeBoundCallback(qsize_event, switch_id));
#endif
				}
				//}
			}
		}
	}
	//core switches
#ifdef NS3_MPI
	cores.Create(m_k * m_k /4, m_k);
#else
	cores.Create(m_k * m_k /4);
#endif
	stack.Install(cores);
	uint32_t core_idx = 0;
	for (uint32_t agg_idx = m_k/2; agg_idx < m_k; ++agg_idx){
		for (uint32_t i = 0; i < m_k/2; ++i){
			for (uint32_t pod_idx = 0; pod_idx < m_k; ++pod_idx){
				tmpdvc = p2p_factory.Install(pods[pod_idx].Get(agg_idx), cores.Get(core_idx));	
				ipv4addr_factory.NewNetwork();
				ipv4addr_factory.Assign(tmpdvc);
				//TODO:Trace
				//if ( Context.systemId == m_k ) {
				for (uint32_t pair = 0; pair < 2; ++pair){
					uint32_t switch_id = tmpdvc.Get(pair)->GetNode()->GetId();
					txq = tmpdvc.Get(pair)->GetObject<PointToPointNetDevice>()->GetQueue();
					txq->TraceConnectWithoutContext("Enqueue",MakeBoundCallback(enq_event, switch_id, tmpdvc.Get(pair)->GetIfIndex(), txq->GetObject<QueueBase>()));
#if DEBUG_LABEL
					txq->GetObject<QueueBase>()->TraceConnectWithoutContext("PacketsInQueue", MakeBoundCallback(qsize_event, switch_id));
#endif
				}
				//}
			}
			core_idx++;
		}
	}
	//hosts
	for (uint32_t pod_idx = 0; pod_idx < m_k; ++pod_idx){//pods
		for (uint32_t edge_idx = 0; edge_idx < m_k/2; ++edge_idx){//edge switches
			NodeContainer tor;
#ifdef NS3_MPI
			tor.Create(m_k/2, pod_idx);
#else
			tor.Create(m_k/2);
#endif
			stack.Install(tor);
			hosts.Add(tor);
			for(uint32_t host_idx = 0; host_idx < m_k/2; ++host_idx){
				tmpdvc = p2p_factory.Install(pods[pod_idx].Get(edge_idx), tor.Get(host_idx));
				ipv4addr_factory.NewNetwork();
				Ipv4InterfaceContainer tmpifc = ipv4addr_factory.Assign(tmpdvc);
				hosts_addr.Add(tmpifc.Get(1));
				//TODO:Trace
				//if ( Context.systemId == pod_idx ) {
				uint32_t switch_id = tmpdvc.Get(0)->GetNode()->GetId();
				txq = tmpdvc.Get(0)->GetObject<PointToPointNetDevice>()->GetQueue();
				txq->TraceConnectWithoutContext("Enqueue",MakeBoundCallback(enq_event, switch_id, tmpdvc.Get(0)->GetIfIndex(), txq->GetObject<QueueBase>()));
#if DEBUG_LABEL
				txq->GetObject<QueueBase>()->TraceConnectWithoutContext("PacketsInQueue", MakeBoundCallback(qsize_event, switch_id));
#endif
				//}
			}
		}
	}
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

uint32_t Fattree::GetHostNumber(){
	return hosts.GetN();
}
uint32_t Fattree::GetSwitchNumber(){
	return m_nSwitch;
}
uint32_t Fattree::GetHostNumberInPod(){
	return m_nHip;
}
Ptr<Node> Fattree::GetHost(uint32_t agg_idx){
	return hosts.Get(agg_idx);
}
Ptr<Node> Fattree::GetHost(uint32_t pod_idx, uint32_t idx){
	return GetHost(idx + (m_k*m_k/4) *pod_idx);
}
Ptr<Node> Fattree::GetHost(uint32_t pod_idx, uint32_t tor_idx, uint32_t idx){
	return GetHost(idx + m_k/2 * tor_idx + (m_k*m_k/4) *pod_idx);
}
Ipv4Address Fattree::GetAddress(uint32_t agg_idx){
	return hosts_addr.GetAddress(agg_idx);
}
Ipv4Address Fattree::GetAddress(uint32_t pod_idx, uint32_t tor_idx, uint32_t idx){
	return hosts_addr.GetAddress(idx + m_k/2 * tor_idx + (m_k*m_k/4) *pod_idx);
}


int main (int argc, char **argv){

	Time::SetResolution (Time::NS);
	Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(1));

	CommandLine cmd;

	uint32_t k = 4;
	cmd.AddValue("k", "number of Ports on a switch", k);

	cmd.Parse(argc, argv);

#ifdef NS3_MPI
	GlobalValue::Bind ("SimulatorImplementationType",
			StringValue ("ns3::NullMessageSimulatorImpl"));
	MpiInterface::Enable (&argc, &argv);
	Context.systemId = MpiInterface::GetSystemId ();
	Context.systemCount = MpiInterface::GetSize ();
#endif

	Fattree ft(k);
#ifdef NS3_MPI
	if( Context.systemId == 0){
#endif
		std::cout << "switchs' id ranges from [ 0, " << ft.GetHost(0)->GetId() << " )" << std::endl;
		std::cout << ft.GetHostNumber() << " hosts created, which id ranges from [ " << ft.GetHost(0)->GetId() << ", " << ft.GetHost(ft.GetHostNumber()-1)->GetId() << " ]" << std::endl;
#ifdef NS3_MPI
	}
#endif


	for (uint32_t pidx = 0; pidx < k; ++pidx){
#ifdef NS3_MPI
		if (Context.systemId == pidx){
#endif
			//Uniform RV, range [Min, Max], used for change app start time.
			Ptr<UniformRandomVariable> uni_rv = CreateObject<UniformRandomVariable> ();
			uni_rv->SetAttribute("Min", DoubleValue(1));
			uni_rv->SetAttribute("Max", DoubleValue(SIMULATION_TIME-1));
			//Pareto RV, used for simulating the characteristic of tcp traffic, the expecation of pareto dist is frac{shape*scale}{shape-1}
			Ptr<ParetoRandomVariable> prt_rv = CreateObject<ParetoRandomVariable> ();
			prt_rv->SetAttribute ("Scale", DoubleValue (8000));
			prt_rv->SetAttribute ("Shape", DoubleValue (2));

			//Exponential distribution RV, used for varying the number of flows from a host to another.
			Ptr<ExponentialRandomVariable> exp_rv = CreateObject<ExponentialRandomVariable> ();
			exp_rv->SetAttribute ("Mean", DoubleValue (1000));
			//exp_rv->SetAttribute ("Bound", DoubleValue ());

			uint32_t listenPort = 80;	

			PacketSinkHelper sink_tcp_factory ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), listenPort));
			ApplicationContainer sinkApp;
			BulkSendHelper bulk_tcp_factory("ns3::TcpSocketFactory", Address(InetSocketAddress(ft.GetAddress(0, 0, 0), listenPort)));
			ApplicationContainer srcApp;

			for (uint32_t hidx = 0; hidx < ft.GetHostNumberInPod(); ++hidx){
				sinkApp.Add(sink_tcp_factory.Install(ft.GetHost(pidx, hidx)));	
				for (uint32_t idx = 0; idx < ft.GetHostNumber(); ++idx){
					if (idx == (pidx * ft.GetHostNumberInPod() + hidx))
						continue;
					bulk_tcp_factory.SetAttribute ("Remote", AddressValue (Address(InetSocketAddress(ft.GetAddress(idx), listenPort))));

					//uint32_t number_of_flows = exp_rv->GetInteger();
					uint32_t number_of_flows = 1000;
					for (uint32_t nof = 0; nof < number_of_flows; ++nof){
						bulk_tcp_factory.SetAttribute("MaxBytes", UintegerValue(prt_rv->GetInteger()));
						srcApp = bulk_tcp_factory.Install(ft.GetHost(hidx));
						srcApp.Start(Seconds(uni_rv->GetValue()));
					}
				}
			}
			sinkApp.Start(Seconds(0.5));
#ifdef NS3_MPI
		}
#endif
	}

	Simulator::Stop(Seconds(SIMULATION_TIME));
	Simulator::Run();
	Simulator::Destroy();

#ifdef NS3_MPI
	MpiInterface::Disable ();
#endif
	return 0;
}
