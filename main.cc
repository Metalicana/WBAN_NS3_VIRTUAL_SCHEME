#include <fstream>
#include <stdlib.h>
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
#include "ns3/point-to-point-star.h"

#include "DoctorRegisterApp.h"
#include "GatewayApp.h"
#include "PhoneApp.h"

#define DOCTOR_REGISTER 5
#define GATEWAY 6
#define PATIENT 7

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

  //system("/home/radivm/Desktop/ns-allinone-3.33/ns-3.33/scratch/wban_final/gen.sh");
  LogComponentEnable("DoctorRegisterApp",LOG_LEVEL_ALL);
  LogComponentEnable("GatewayApp",LOG_LEVEL_ALL);
  LogComponentEnable("PhoneApp",LOG_LEVEL_ALL);
  int doctorNodeCnt=5;
  int gatewayNodeCnt=1;
  int patientNodeCnt=3;
  int sensorNodeCnt=patientNodeCnt*3;
  int totalNodes = doctorNodeCnt+gatewayNodeCnt+sensorNodeCnt+patientNodeCnt;

  NodeContainer GatewayNode;
  NodeContainer DoctorNodes;
  NodeContainer PatientNodes;
  NodeContainer SensorNodes;
  NodeContainer nodes;

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  PointToPointStarHelper p2pStar(patientNodeCnt+sensorNodeCnt+doctorNodeCnt,pointToPoint);



  NetDeviceContainer devices;
  NodeContainer GatewayNetDevice;
  NodeContainer DoctorNetDevices;
  NodeContainer PatientNetDevices;
  NodeContainer SensorNetDevices;



  InternetStackHelper stack;
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer interfaces;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  p2pStar.InstallStack(stack);
  
  p2pStar.AssignIpv4Addresses(address);
  uint16_t sinkPort = 8080;
  
  Ptr<Socket> doctor_listener[doctorNodeCnt];
  Ptr<Socket> doctor_speaker[doctorNodeCnt];
  Ptr<Socket> patient_listener[patientNodeCnt];
  Ptr<Socket> patient_speaker[patientNodeCnt];
  int i=0;
  for(int j=0;j<doctorNodeCnt;i++,j++)
  {
    doctor_listener[j] = Socket::CreateSocket ( p2pStar.GetSpokeNode(i),TcpSocketFactory::GetTypeId ());
    doctor_speaker[j]  = Socket::CreateSocket ( p2pStar.GetSpokeNode(i),TcpSocketFactory::GetTypeId ());
  }
  i++;
  for(int j=0;j<patientNodeCnt;i++,j++)
  {
    patient_listener[j] = Socket::CreateSocket ( p2pStar.GetSpokeNode(i),TcpSocketFactory::GetTypeId ());
    patient_speaker[j]  = Socket::CreateSocket ( p2pStar.GetSpokeNode(i),TcpSocketFactory::GetTypeId ());
  }
  Ptr<Socket> gw_listener = Socket::CreateSocket (p2pStar.GetHub(), TcpSocketFactory::GetTypeId ());
  Ptr<Socket> gw_speaker = Socket::CreateSocket (p2pStar.GetHub(), TcpSocketFactory::GetTypeId ());

  Address doctorAddress[doctorNodeCnt];
  Address patientAddress[patientNodeCnt];
  Address gwAddress[totalNodes-1];
  for(int j=0;j<totalNodes-1;j++)
  {
    gwAddress[j] = InetSocketAddress(p2pStar.GetHubIpv4Address(j), sinkPort);
  }
  i=0;
  for(int j=0;j<doctorNodeCnt;i++,j++)
  {
    doctorAddress[j]=InetSocketAddress (p2pStar.GetSpokeIpv4Address (i), sinkPort);
  }
  
  for(int j=0;j<patientNodeCnt;i++,j++)
  {
    patientAddress[j] =InetSocketAddress (p2pStar.GetSpokeIpv4Address (i), sinkPort);
  }

  Ptr<DoctorRegisterApp> doctor_app[doctorNodeCnt];
  Ptr<PhoneApp> phone_app[patientNodeCnt];
  i=0;
  for(int j=0;j<doctorNodeCnt;i++,j++)
  {
    doctor_app[j] = CreateObject<DoctorRegisterApp> ();
    doctor_app[j]->SetStartTime (Seconds (1.));
    doctor_app[j]->SetStopTime (Seconds (20.));
    doctor_app[j]->Setup (doctor_speaker[j],doctor_listener[j], gwAddress[i] , 1040, 1000, DataRate ("1Mbps"),sinkPort,DOCTOR_REGISTER);
    (p2pStar.GetSpokeNode(i))->AddApplication(doctor_app[j]);
  }
  for(int j=0;j<patientNodeCnt;j++,i++)
  {
    phone_app[j] = CreateObject<PhoneApp> ();
    phone_app[j]->SetStartTime (Seconds (1.));
    phone_app[i]->SetStopTime (Seconds (20.));
    phone_app[j]->Setup (patient_speaker[j],patient_listener[j], gwAddress[i]  , 1040, 1000, DataRate ("1Mbps"),sinkPort,PATIENT);
    (p2pStar.GetSpokeNode(i))->AddApplication(phone_app[j]);
  }
  

  Ptr<GatewayApp> gw_app = CreateObject<GatewayApp> ();
  gw_app->SetStartTime (Seconds (1.));
  gw_app->SetStopTime (Seconds (20.));
  gw_app->Setup (gw_speaker,gw_listener, p2pStar.GetHubIpv4Address(0) , 1040, 1000, DataRate ("1Mbps"),sinkPort,GATEWAY,10);
  (p2pStar.GetHub())->AddApplication(gw_app);

  //doctor_app[0]->StartSending("591022132\nabcdefgh");
  Simulator::Schedule (Seconds (2), &DoctorRegisterApp::SendRegisterInfo, doctor_app[0], "1000" , "abcde", gwAddress[0]);


//   Ipv4AddressHelper address;
//   address.SetBase ("10.1.1.0", "255.255.255.0");
//   Ipv4InterfaceContainer interfaces = address.Assign (devices);

//   uint16_t sinkPort = 8080;
  
//   /*Address doctorAddress[doctorNodes];
//   Address patientAddress[patientNodes];
//   Address sensorAddress[sensorNodes];
//   int i=0;
//   for(int j=0;j<doctorNodes;i++,j++)
//   {
//     doctorAddress[j]=InetSocketAddress (interfaces.GetAddress (i), sinkPort);
//   }
//   Address gwAddress = InetSocketAddress(interfaces.GetAddress (i), sinkPort);
//   i++;
//   for(int j=0;j<patientNodes;i++,j++)
//   {
//     patientAddress[j] =InetSocketAddress (interfaces.GetAddress (i), sinkPort);
//   }
//   for(int j=0;j<sensorNodes;i++,j++)
//   {
//     sensorAddress[j] = InetSocketAddress (interfaces.GetAddress (i), sinkPort);
//   }*/
//    // 0 is doctor 1 is gw
//    //(InetSocketAddress (interfaces.GetAddress (i), sinkPort));
//   Address doctorAddress (InetSocketAddress (interfaces.GetAddress (0), sinkPort));
//   Address gwAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));

//   Ptr<Socket> doctor_listener = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
//   Ptr<Socket> doctor_speaker = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());

  
//  /* Ptr<Socket> doctor_listener[doctorNodes];
//   Ptr<Socket> doctor_speaker[doctorNodes];
//   for(int i=0;i<doctorNodes;i++)
//   {
//     doctor_listener[i] = Socket::CreateSocket (nodes.Get (i), TcpSocketFactory::GetTypeId ());
//     doctor_speaker[i]  = Socket::CreateSocket (nodes.Get (i), TcpSocketFactory::GetTypeId ());
//   }*/
//   Ptr<Socket> gw_listener = Socket::CreateSocket (nodes.Get (1), TcpSocketFactory::GetTypeId ());
//   Ptr<Socket> gw_speaker = Socket::CreateSocket (nodes.Get (1), TcpSocketFactory::GetTypeId ());

//   /*doctor_listener->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));
//   gw_listener->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));
//   gw_speaker->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));
//   doctor_speaker->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));*/
//   //Ptr<DoctorRegisterApp> doctor_app[doctorNodes];
//   //for(int i=0;i<doctorNodes;i++)doctor_app[i] = CreateObject<DoctorRegisterApp> ();
//   Ptr<DoctorRegisterApp> doctor_app = CreateObject<DoctorRegisterApp> ();
//   Ptr<GatewayApp> gw_app = CreateObject<GatewayApp> ();

//   doctor_app->SetStartTime (Seconds (1.));
//   doctor_app->SetStopTime (Seconds (20.));
  
//   gw_app->SetStartTime (Seconds (1.));
//   gw_app->SetStopTime (Seconds (20.));

//   doctor_app->Setup (doctor_speaker,doctor_listener, gwAddress, 1040, 1000, DataRate ("1Mbps"),sinkPort,5);
//   gw_app->Setup (gw_speaker,gw_listener, doctorAddress, 1040, 1000, DataRate ("1Mbps"),sinkPort,6,10);

//   nodes.Get (0)->AddApplication (doctor_app);
//   nodes.Get (1)->AddApplication (gw_app);
//   //take some input from the user and send them to the gateway
//   //Gateway will then process it and return the good stuff.
//   doctor_app->StartSending("591022132\nabcdefgh");

  
//   Ptr <Packet> packet1 = Create <Packet> (400);
//   Ptr <Packet> packet2 = Create <Packet> (800);
//   doctor_app->SendPacket(packet1);
//   Simulator::Schedule (Seconds (2), &DoctorRegisterApp::SendRegisterInfo, doctor_app[0], "1000\n" , "abcde", doctorAddress);
//   Simulator::Schedule (Seconds (2), &DoctorRegisterApp::SendPacket, doctor_app ,packet2);
//  Simulator::Schedule (Seconds (2), &MyApp::SendPacket,doctor_app);
//   /*devices.Get (0)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
//   devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));*/

  Simulator::Stop (Seconds (20));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
