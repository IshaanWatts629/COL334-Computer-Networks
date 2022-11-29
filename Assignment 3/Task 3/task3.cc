#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Task3");

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
}

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  NodeContainer nodes;
  nodes.Create (5);

  NodeContainer n1n2 = NodeContainer(nodes.Get(0), nodes.Get(1));
  NodeContainer n2n3 = NodeContainer(nodes.Get(1), nodes.Get(2));
  NodeContainer n3n4 = NodeContainer(nodes.Get(2), nodes.Get(3));
  NodeContainer n3n5 = NodeContainer(nodes.Get(2), nodes.Get(4));

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer d1d2 = pointToPoint.Install(n1n2);
  NetDeviceContainer d2d3 = pointToPoint.Install(n2n3);
  NetDeviceContainer d3d4 = pointToPoint.Install(n3n4);
  NetDeviceContainer d3d5 = pointToPoint.Install(n3n5);

  string protocol = "ns3::TcpVegas";
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue (protocol));
  
  InternetStackHelper stack;
  stack.Install (nodes);
  
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = address.Assign (d1d2);
  
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i3 = address.Assign (d2d3);
  
  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i4 = address.Assign (d3d4);
  
  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i5 = address.Assign (d3d5);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  //UDP connection between N2 --> N3 --> N5
  uint16_t sinkPortUDP = 8090;
  Address sinkAddressUDP (InetSocketAddress(i3i5.GetAddress (1), sinkPortUDP));
  PacketSinkHelper packetSinkHelperUDP ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPortUDP));
  ApplicationContainer sinkAppsUDP = packetSinkHelperUDP.Install (nodes.Get (4));
  sinkAppsUDP.Start (Seconds(20.0));
  sinkAppsUDP.Stop (Seconds(100.0));

  Ptr<Socket> ns3UdpSocket1 = Socket::CreateSocket (nodes.Get(1), UdpSocketFactory::GetTypeId ());

  Ptr<MyApp> app1 = CreateObject<MyApp> ();
  app1->Setup(ns3UdpSocket1, sinkAddressUDP, 1040, 100000, DataRate("250Kbps"));
  nodes.Get (1)->AddApplication (app1);
  app1->SetStartTime (Seconds(20.0));
  app1->SetStopTime (Seconds(30.0));

  Ptr<Socket> ns3UdpSocket2 = Socket::CreateSocket (nodes.Get(1), UdpSocketFactory::GetTypeId ());

  Ptr<MyApp> app2 = CreateObject<MyApp> ();
  app2->Setup(ns3UdpSocket2, sinkAddressUDP, 1040, 100000, DataRate("500Kbps"));
  nodes.Get (1)->AddApplication (app2);
  app2->SetStartTime (Seconds(30.0));
  app2->SetStopTime (Seconds(100.0));

  //TCP connection between N1 --> N2 --> N3 --> N4
  uint16_t sinkPortTCP = 8080;
  Address sinkAddressTCP (InetSocketAddress (i3i4.GetAddress (1), sinkPortTCP));
  PacketSinkHelper packetSinkHelperTCP ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPortTCP));
  ApplicationContainer sinkAppsTCP = packetSinkHelperTCP.Install (nodes.Get (3));
  sinkAppsTCP.Start (Seconds (1.));
  sinkAppsTCP.Stop (Seconds (100.));

  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddressTCP, 1040, 100000, DataRate ("250Kbps"));
  nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (100.));

  //Data Collection
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("task3.csv");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

  PcapHelper pcapHelper;
  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("task3.pcap", std::ios::out, PcapHelper::DLT_PPP);
  d1d2.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop, file));
  
  pointToPoint.EnablePcapAll("task3");

  Simulator::Stop (Seconds (100));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

