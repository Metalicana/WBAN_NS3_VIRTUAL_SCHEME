#include <fstream>
#include <string>
#include <vector>
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
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/point-to-point-star.h"
#include "ns3/error-model.h"
#include "DoctorRegisterApp.h"
#include "GatewayApp.h"
#include "PhoneApp.h"
#include "SensorApp.h"
#include "ns3/athstats-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"

#define DOCTOR_REGISTER 5
#define GATEWAY 6
#define PATIENT 7
#define SENSOR 8

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

void init( Ptr<SensorApp> s,Ptr<PhoneApp> p, Address ad)
{
  //get the Ui
  byte *buff = p->GetUi();
  InetSocketAddress add = InetSocketAddress::ConvertFrom (ad);
  p->UpdateSensor(ad);
  s->SendPing(buff);
}
void trace(YansWifiPhyHelper a)
{
  AsciiTraceHelper ascii;
  a.EnableAsciiAll(ascii.CreateFileStream("b.tr"));
}
int main(int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);
  double simulationStopTime=1200.0;
  ns3::PacketMetadata::Enable ();
  // LogComponentEnable("DoctorRegisterApp",LOG_LEVEL_ALL);
  //  LogComponentEnable("GatewayApp",LOG_LEVEL_ALL);
  //  LogComponentEnable("PhoneApp",LOG_LEVEL_ALL);
  //  LogComponentEnable("SensorApp",LOG_LEVEL_ALL);

  uint16_t sinkPort = 8080;

  int doctorNodeCnt=2;
  int gatewayNodeCnt=1;
  int patientNodeCnt=3;
  int perSensorNode = 10;
  int sensorNodeCnt=patientNodeCnt*perSensorNode;
  int totalNodes = doctorNodeCnt+gatewayNodeCnt+sensorNodeCnt+patientNodeCnt;

  NodeContainer doctorNodes;
  doctorNodes.Create(doctorNodeCnt);

  NodeContainer patientNodes;
  patientNodes.Create(patientNodeCnt);

  NodeContainer gatewayNodes;
  gatewayNodes.Create(gatewayNodeCnt);

  NodeContainer sensorNodes;
  sensorNodes.Create(sensorNodeCnt);

  NodeContainer allNodes;
  allNodes.Add(gatewayNodes);
  allNodes.Add(doctorNodes);
  allNodes.Add(patientNodes);
  allNodes.Add(sensorNodes);

  NodeContainer AP;
  AP.Create(1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  wifi.SetStandard(WIFI_STANDARD_80211a);
  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer gatewayDevices;
  gatewayDevices = wifi.Install (phy, mac, gatewayNodes);

  NetDeviceContainer doctorDevices;
  doctorDevices = wifi.Install (phy, mac, doctorNodes);

  NetDeviceContainer patientDevices;
  patientDevices = wifi.Install (phy, mac, patientNodes);

  NetDeviceContainer sensorDevices;
  sensorDevices = wifi.Install (phy, mac, sensorNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, AP);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (.1),
                                 "DeltaY", DoubleValue (.1),
                                 "GridWidth", UintegerValue (500000),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-1000, 1000, -1000, 1000)));
  mobility.Install (allNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (AP);
  mobility.Install(allNodes);
  InternetStackHelper stack;
  stack.Install (AP);
  stack.Install (allNodes);
  
  

  Ipv4AddressHelper address;
  
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer gatewayInterface;
  gatewayInterface = address.Assign (gatewayDevices);

  Ipv4InterfaceContainer doctorInterface;
  doctorInterface = address.Assign (doctorDevices);

  Ipv4InterfaceContainer patientInterface;
  patientInterface = address.Assign (patientDevices);

  Ipv4InterfaceContainer sensorInterface;
  sensorInterface = address.Assign (sensorDevices);

  Ipv4InterfaceContainer APInterface;
  APInterface = address.Assign (apDevices);
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
  Ptr<Socket> doctor_listener[doctorNodeCnt];
  Ptr<Socket> doctor_speaker[doctorNodeCnt];

  Ptr<Socket> patient_listener[patientNodeCnt];
  Ptr<Socket> patient_speaker[patientNodeCnt];

  Ptr<Socket> sensor_listener[sensorNodeCnt];
  Ptr<Socket> sensor_speaker[sensorNodeCnt];

  for(int j=0;j<doctorNodeCnt;j++)
  {
    doctor_listener[j] = Socket::CreateSocket ( doctorNodes.Get(j),TcpSocketFactory::GetTypeId ());
    doctor_speaker[j]  = Socket::CreateSocket ( doctorNodes.Get(j),TcpSocketFactory::GetTypeId ());
  }
  for(int j=0;j<patientNodeCnt;j++)
  {
    patient_listener[j] = Socket::CreateSocket ( patientNodes.Get(j),TcpSocketFactory::GetTypeId ());
    patient_speaker[j]  = Socket::CreateSocket ( patientNodes.Get(j),TcpSocketFactory::GetTypeId ());
  }
  for(int j=0;j<sensorNodeCnt;j++)
  {
    sensor_listener[j] = Socket::CreateSocket ( sensorNodes.Get(j),TcpSocketFactory::GetTypeId ());
    sensor_speaker[j]  = Socket::CreateSocket ( sensorNodes.Get(j),TcpSocketFactory::GetTypeId ());
  }
  Ptr<Socket> gw_listener = Socket::CreateSocket (gatewayNodes.Get(0), TcpSocketFactory::GetTypeId ());
  Ptr<Socket> gw_speaker = Socket::CreateSocket (gatewayNodes.Get(0), TcpSocketFactory::GetTypeId ());

  Address doctorAddress[doctorNodeCnt];
  Address patientAddress[patientNodeCnt];
  Address gatewayAddress;
  Address sensorAddress[sensorNodeCnt];
  Address APAddress;
  AsciiTraceHelper as;
  //phy.EnableAsciiAll(as.CreateFileStream("a.tr"));
  Simulator::Schedule (Seconds (14.0), &trace,phy);
  gatewayAddress = InetSocketAddress(gatewayInterface.GetAddress(0),sinkPort);
  APAddress = InetSocketAddress(APInterface.GetAddress(0),sinkPort);
  
  for(int j=0;j<doctorNodeCnt;j++)
  {
    doctorAddress[j]=InetSocketAddress (doctorInterface.GetAddress(j), sinkPort);
  }
  for(int j=0;j<patientNodeCnt;j++)
  {
    patientAddress[j] =InetSocketAddress (patientInterface.GetAddress(j), sinkPort);
  }
  for(int j=0;j<sensorNodeCnt;j++)
  {
    sensorAddress[j] =InetSocketAddress (sensorInterface.GetAddress(j), sinkPort);
    //cout << sensorAddress[j]<< endl;
  }

  Ptr<DoctorRegisterApp> doctor_app[doctorNodeCnt];
  Ptr<PhoneApp> phone_app[patientNodeCnt];
  Ptr<SensorApp> sensor_app[sensorNodeCnt];
  Ptr<GatewayApp> gw_app = CreateObject<GatewayApp> ();

  
  for(int j=0;j<doctorNodeCnt;j++)
  {
    doctor_app[j] = CreateObject<DoctorRegisterApp> ();
    doctor_app[j]->SetStartTime (Seconds (1.));
    doctor_app[j]->SetStopTime (Seconds (simulationStopTime));
    doctor_app[j]->Setup (doctor_speaker[j],doctor_listener[j], gatewayAddress , 1040, 1000, DataRate ("1Mbps"),sinkPort,DOCTOR_REGISTER);
    doctorNodes.Get(j)->AddApplication(doctor_app[j]);
  }
  for(int j=0;j<patientNodeCnt;j++)
  {
    phone_app[j] = CreateObject<PhoneApp> ();
    phone_app[j]->SetStartTime (Seconds (1.));
    phone_app[j]->SetStopTime (Seconds (simulationStopTime));
    phone_app[j]->Setup (patient_speaker[j],patient_listener[j], gatewayAddress  , 1040, 1000, DataRate ("1Mbps"),sinkPort,PATIENT);
    patientNodes.Get(j)->AddApplication(phone_app[j]);
  }
  for(int j=0;j<sensorNodeCnt;j++)
  {
    sensor_app[j] = CreateObject<SensorApp> ();
    sensor_app[j]->SetStartTime (Seconds (1.));
    sensor_app[j]->SetStopTime (Seconds (simulationStopTime));
    sensor_app[j]->Setup (sensor_speaker[j],sensor_listener[j], gatewayAddress , 1040, 1000, DataRate ("1Mbps"),sinkPort,SENSOR);
    sensorNodes.Get(j)->AddApplication(sensor_app[j]);
  }

  
  gw_app->SetStartTime (Seconds (1.));
  gw_app->SetStopTime (Seconds (simulationStopTime));
  gw_app->Setup (gw_speaker,gw_listener, gatewayAddress , 1040, 1000, DataRate ("1Mbps"),sinkPort,GATEWAY,10);
  gatewayNodes.Get(0)->AddApplication(gw_app);


  for(int j=0;j<doctorNodeCnt;j++)
  {
    char x = j + (int)'0' + 1;
    string y = "";
    y+=x;
    y+='0';
    y+='0';
    y+='0';
    for(int k=0;k<sensorNodeCnt;k++)
    {   
      doctor_app[j]->SetMid(y);
      sensor_app[k]->AddAddress(y,doctorAddress[j]);
    }
  }
  gatewayNodes.Get(0)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
  Simulator::Schedule (Seconds (3.0), &DoctorRegisterApp::SendRegisterInfo, doctor_app[0], "1000" , "abcde", gatewayAddress);
  Simulator::Schedule (Seconds (3.1), &DoctorRegisterApp::SendRegisterInfo, doctor_app[1], "2000" , "abcde", gatewayAddress);
  // Simulator::Schedule (Seconds (3.2), &DoctorRegisterApp::SendRegisterInfo, doctor_app[2], "3000" , "abcde", gatewayAddress);
  // Simulator::Schedule (Seconds (3.3), &DoctorRegisterApp::SendRegisterInfo, doctor_app[3], "4000" , "abcde", gatewayAddress);
  // Simulator::Schedule (Seconds (3.4), &DoctorRegisterApp::SendRegisterInfo, doctor_app[4], "5000" , "abcde", gatewayAddress);

  Simulator::Schedule (Seconds (6.00),  &PhoneApp::SendPing, phone_app[0]);
  Simulator::Schedule (Seconds (6.001), &PhoneApp::SendPing, phone_app[1]);
 Simulator::Schedule (Seconds (6.002), &PhoneApp::SendPing, phone_app[2]);
  //  Simulator::Schedule (Seconds (6.003), &PhoneApp::SendPing, phone_app[3]);
  //  Simulator::Schedule (Seconds (6.004), &PhoneApp::SendPing, phone_app[4]);

  int q=0;
  for(int i=0;i<patientNodeCnt;i++)
  {
    for(int j=0;j<perSensorNode;j++)
    {
      //sensor node will ping the server with UI of the patient
      //server will handle it
      Simulator::Schedule (Seconds (7.0 + (double)q), &init, sensor_app[i*perSensorNode+j], phone_app[i],sensorAddress[i*perSensorNode+j]);
      q++;
    }
  }
  AsciiTraceHelper ascii;
  // Ptr<FlowMonitor> flowMonitor;
  // FlowMonitorHelper flowHelper;
  // flowMonitor = flowHelper.InstallAll();
 
  for(int q=0;q<450;q++)
  {
    for(int i=0;i<5;i++)
    {
      Simulator::Schedule (Seconds (15.0 + (double)q + (double)i), &DoctorRegisterApp::SendLoginInfo, doctor_app[0], "1000" , "abcde", gatewayAddress, phone_app[0], sensor_app[i]);
      Simulator::Schedule (Seconds (15.0 + (double)q + (double)i + 50.0), &DoctorRegisterApp::SendLoginInfo, doctor_app[0], "1000" , "abcde", gatewayAddress, phone_app[1], sensor_app[10+i]);
      Simulator::Schedule (Seconds (15.0 + (double)q + (double)i + 25.0), &DoctorRegisterApp::SendLoginInfo, doctor_app[0], "1000" , "abcde", gatewayAddress, phone_app[2], sensor_app[20+i]);
    }
  }

  
    
  
  
  Simulator::Stop (Seconds (simulationStopTime));
  Simulator::Run ();
  //flowMonitor->SerializeToXmlFile("NameOfFile.xml", true, true);
  Simulator::Destroy ();


  return 0;
}
