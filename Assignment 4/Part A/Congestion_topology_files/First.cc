#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <bits/stdc++.h>

using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Task1");
int dropped = 0;

class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "," << oldCwnd << "," << newCwnd << std::endl;
}

static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
  file->Write (Simulator::Now (), p);
  dropped += 1;
}

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  NodeContainer nodes;
  nodes.Create (3);

  NodeContainer n1n3 = NodeContainer (nodes.Get (0), nodes.Get (2));
  NodeContainer n2n3 = NodeContainer (nodes.Get (1), nodes.Get (2));
   
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("3ms"));

  NetDeviceContainer d1d3 = p2p.Install (n1n3);

  p2p.SetDeviceAttribute ("DataRate", StringValue ("9Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("3ms"));

  NetDeviceContainer d2d3 = p2p.Install (n2n3);

  Ptr<RateErrorModel> em1 = CreateObject<RateErrorModel> ();
  em1->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  d1d3.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));

  Ptr<RateErrorModel> em2 = CreateObject<RateErrorModel> ();
  em2->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  d2d3.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em2));

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i3 = address.Assign (d1d3);
  
  address.SetBase ("10.0.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i3 = address.Assign (d2d3);

  uint16_t sinkPort1 = 8080;
  Address sinkAddress1 (InetSocketAddress (i1i3.GetAddress (1), sinkPort1));
  PacketSinkHelper packetSinkHelper1 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort1));
  ApplicationContainer sinkApps1 = packetSinkHelper1.Install (nodes.Get (2));
  sinkApps1.Start (Seconds (1.));
  sinkApps1.Stop (Seconds (30.));
  
  uint16_t sinkPort2 = 8090;
  Address sinkAddress2 (InetSocketAddress (i1i3.GetAddress (1), sinkPort2));
  PacketSinkHelper packetSinkHelper2 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort2));
  ApplicationContainer sinkApps2 = packetSinkHelper2.Install (nodes.Get (2));
  sinkApps2.Start (Seconds (1.));
  sinkApps2.Stop (Seconds (30.));
  
  uint16_t sinkPort3 = 8100;
  Address sinkAddress3 (InetSocketAddress (i2i3.GetAddress (1), sinkPort3));
  PacketSinkHelper packetSinkHelper3 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort3));
  ApplicationContainer sinkApps3 = packetSinkHelper3.Install (nodes.Get (2));
  sinkApps3.Start (Seconds (1.));
  sinkApps3.Stop (Seconds (30.));
  
  //Choose Configuration
  int config;
  cout << "Enter Configuration (1/2): ";
  cin >> config;

  TypeId tid;

  if(config == 1){
        tid = TypeId::LookupByName ("ns3::TcpNewReno");
  }
  else if(config == 2){
        tid = TypeId::LookupByName ("ns3::TcpNewRenoPlus");
  }
  else{
        cout << "Invalid Configuration" << endl;
        exit(-1);
  }
  
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue (tid));
  
  //For Node1
  Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  Ptr<Socket> ns3TcpSocket2 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> app1 = CreateObject<MyApp> ();
  app1->Setup (ns3TcpSocket1, sinkAddress1, 3000, 15000, DataRate ("1.5Mbps"));
  n1n3.Get (0)->AddApplication (app1);
  app1->SetStartTime (Seconds (1.));
  app1->SetStopTime (Seconds (20.));
  
  Ptr<MyApp> app2 = CreateObject<MyApp> ();
  app2->Setup (ns3TcpSocket2, sinkAddress2, 3000, 15000, DataRate ("1.5Mbps"));
  n1n3.Get (0)->AddApplication (app2);
  app2->SetStartTime (Seconds (5.));
  app2->SetStopTime (Seconds (25.));

  //For Node2
  Ptr<Socket> ns3TcpSocket3 = Socket::CreateSocket (nodes.Get (1), TcpSocketFactory::GetTypeId ());
  
  Ptr<MyApp> app3 = CreateObject<MyApp> ();
  app3->Setup (ns3TcpSocket3, sinkAddress3, 3000, 15000, DataRate ("1.5Mbps"));
  n2n3.Get (0)->AddApplication (app3);
  app3->SetStartTime (Seconds (15.));
  app3->SetStopTime (Seconds (30.));
  
  //Data Collection
  string save1 = "task1_connection1_" + to_string(config) + ".csv";
  string save2 = "task1_connection2_" + to_string(config) + ".csv";
  string save3 = "task1_connection3_" + to_string(config) + ".csv";

  AsciiTraceHelper asciiTraceHelper;

  Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (save1);
  ns3TcpSocket1->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream1));
  
  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (save2);
  ns3TcpSocket2->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream2));
  
  Ptr<OutputStreamWrapper> stream3 = asciiTraceHelper.CreateFileStream (save3);
  ns3TcpSocket3->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream3));
  
  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file1 = pcapHelper.CreateFile ("task1_d1d3.pcap", std::ios::out, PcapHelper::DLT_PPP);
  d1d3.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file1));
  
  Ptr<PcapFileWrapper> file2 = pcapHelper.CreateFile ("task1_d2d3.pcap", std::ios::out, PcapHelper::DLT_PPP);
  d2d3.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file2));
  
  Simulator::Stop (Seconds (30));
  Simulator::Run ();
  Simulator::Destroy ();
  cout << "Dropped Packets: " << dropped << endl;
  
  return 0;
}
