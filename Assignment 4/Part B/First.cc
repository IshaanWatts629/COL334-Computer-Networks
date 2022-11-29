#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("RipSimpleRouting");

void TearDownLink (Ptr<Node> nodeA, Ptr<Node> nodeB, uint32_t interfaceA, uint32_t interfaceB)
{
  nodeA->GetObject<Ipv4> ()->SetDown (interfaceA);
  nodeB->GetObject<Ipv4> ()->SetDown (interfaceB);
}

int main (int argc, char **argv)
{
  bool verbose = false;
  bool printRoutingTables = true;
  bool showPings = false;

  CommandLine cmd;
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.AddValue ("printRoutingTables", "Print routing tables at 30, 60 and 90 seconds", printRoutingTables);
  cmd.AddValue ("showPings", "Show Ping6 reception", showPings);
  cmd.Parse (argc, argv);

  if (verbose)
    {
      LogComponentEnableAll (LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE));
      LogComponentEnable ("RipSimpleRouting", LOG_LEVEL_INFO);
      LogComponentEnable ("Rip", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv4Interface", LOG_LEVEL_ALL);
      LogComponentEnable ("Icmpv4L4Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("ArpCache", LOG_LEVEL_ALL);
      LogComponentEnable ("V4Ping", LOG_LEVEL_ALL);
    }

  Config::SetDefault ("ns3::Rip::SplitHorizon", EnumValue (RipNg::SPLIT_HORIZON));
  
  NS_LOG_INFO ("Create nodes.");

  Ptr<Node> src = CreateObject<Node> ();
  Names::Add ("SrcNode", src);
  Ptr<Node> dst = CreateObject<Node> ();
  Names::Add ("DstNode", dst);

  Ptr<Node> r1 = CreateObject<Node> ();
  Names::Add ("Router1", r1);
  Ptr<Node> r2 = CreateObject<Node> ();
  Names::Add ("Router2", r2);
  Ptr<Node> r3 = CreateObject<Node> ();
  Names::Add ("Router3", r3);
  
  NodeContainer s_r1 (src, r1);
  NodeContainer r1r2 (r1, r2);
  NodeContainer r1r3 (r1, r3);
  NodeContainer r2r3 (r2, r3);
  NodeContainer r3_d (r3, dst);

  NodeContainer routers (r1, r2, r3);
  NodeContainer nodes (src, dst);

  NS_LOG_INFO ("Create channels.");
  CsmaHelper csma;

  int delay;
  cout << "Enter Delay (ms): ";
  cin >> delay;

  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (delay)));

  NetDeviceContainer ndc1 = csma.Install (s_r1);
  NetDeviceContainer ndc2 = csma.Install (r1r2);
  NetDeviceContainer ndc3 = csma.Install (r1r3);
  NetDeviceContainer ndc4 = csma.Install (r2r3);
  NetDeviceContainer ndc5 = csma.Install (r3_d);

  NS_LOG_INFO ("Create IPv4 and routing");
  RipHelper ripRouting;
  
  ripRouting.ExcludeInterface (r1, 1);
  ripRouting.ExcludeInterface (r3, 3);

  Ipv4ListRoutingHelper listRH;
  listRH.Add (ripRouting, 0);

  InternetStackHelper internet;
  internet.SetIpv6StackInstall (false);
  internet.SetRoutingHelper (listRH);
  internet.Install (routers);

  InternetStackHelper internetNodes;
  internetNodes.SetIpv6StackInstall (false);
  internetNodes.Install (nodes);
  
  NS_LOG_INFO ("Assign IPv4 Addresses.");
  Ipv4AddressHelper ipv4;

  ipv4.SetBase (Ipv4Address ("10.0.0.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic1 = ipv4.Assign (ndc1);

  ipv4.SetBase (Ipv4Address ("10.0.1.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic2 = ipv4.Assign (ndc2);

  ipv4.SetBase (Ipv4Address ("10.0.2.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic3 = ipv4.Assign (ndc3);

  ipv4.SetBase (Ipv4Address ("10.0.3.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic4 = ipv4.Assign (ndc4);

  ipv4.SetBase (Ipv4Address ("10.0.4.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic5 = ipv4.Assign (ndc5);


  Ptr<Ipv4StaticRouting> staticRouting;

  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (src->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("10.0.0.2", 1 );
  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (dst->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("10.0.4.1", 1 );

  if (printRoutingTables)
    {
      RipHelper routingHelper;

      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);

      routingHelper.PrintRoutingTableAt (Seconds (0.0), r1, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (0.0), r2, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (0.0), r3, routingStream);

      routingHelper.PrintRoutingTableAt (Seconds (10.0), r1, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (10.0), r2, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (10.0), r3, routingStream);

      routingHelper.PrintRoutingTableAt (Seconds (50.0), r1, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (50.0), r2, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (50.0), r3, routingStream);

      routingHelper.PrintRoutingTableAt (Seconds (100.0), r1, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (100.0), r2, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (100.0), r3, routingStream);

      routingHelper.PrintRoutingTableAt (Seconds (500.0), r1, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (500.0), r2, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (500.0), r3, routingStream);
    }

  NS_LOG_INFO ("Create Applications.");

  uint32_t packetSize = 1024;
  Time interPacketInterval = Seconds (1.0);
  V4PingHelper ping ("10.0.4.2");

  ping.SetAttribute ("Interval", TimeValue (interPacketInterval));
  ping.SetAttribute ("Size", UintegerValue (packetSize));

  if (showPings)
    {
      ping.SetAttribute ("Verbose", BooleanValue (true));
    }

  ApplicationContainer apps = ping.Install (src);
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (500.0));

  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("rip-simple-routing.tr"));
  csma.EnablePcapAll ("rip-simple-routing", true);

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (500.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}

