#include <fstream>
#include "ns3/log.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/tcp-header.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"

#include "DoctorRegisterApp.h"
#include "GatewayApp.h"

#define DOCTOR_REGISTER 5
#define GATEWAY 6

using namespace std;
using namespace ns3;

static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
}

static void
RxDrop (Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}


int 
main (int argc, char *argv[])
{

  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);

  LogComponentEnable("DoctorRegisterApp",LOG_LEVEL_ALL);
  LogComponentEnable("GatewayApp",LOG_LEVEL_ALL);
  
  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);


  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  uint16_t sinkPort = 8080;
  
  Address doctorAddress (InetSocketAddress (interfaces.GetAddress (0), sinkPort));// 0 is doctor 1 is gw
  Address gwAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));

  Ptr<Socket> doctor_listener = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  Ptr<Socket> gw_listener = Socket::CreateSocket (nodes.Get (1), TcpSocketFactory::GetTypeId ());
  Ptr<Socket> doctor_speaker = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  Ptr<Socket> gw_speaker = Socket::CreateSocket (nodes.Get (1), TcpSocketFactory::GetTypeId ());

  
  doctor_listener->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));
  gw_listener->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));
  gw_speaker->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));
  doctor_speaker->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));

  Ptr<DoctorRegisterApp> doctor_app = CreateObject<DoctorRegisterApp> ();
  Ptr<GatewayApp> gw_app = CreateObject<GatewayApp> ();

  doctor_app->SetStartTime (Seconds (1.));
  doctor_app->SetStopTime (Seconds (20.));
  
  gw_app->SetStartTime (Seconds (1.));
  gw_app->SetStopTime (Seconds (20.));

  doctor_app->Setup (doctor_speaker,doctor_listener, gwAddress, 1040, 1000, DataRate ("1Mbps"),sinkPort,5);
  gw_app->Setup (gw_speaker,gw_listener, doctorAddress, 1040, 1000, DataRate ("1Mbps"),sinkPort,6);

  nodes.Get (0)->AddApplication (doctor_app);
  nodes.Get (1)->AddApplication (gw_app);
  //take some input from the user and send them to the gateway
  //Gateway will then process it and return the good stuff.
  doctor_app->StartSending("571022132\nabcdefgh");
  
 // Simulator::Schedule (Seconds (2), &MyApp::SendPacket,doctor_app);
  devices.Get (0)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
  devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));

  Simulator::Stop (Seconds (20));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}