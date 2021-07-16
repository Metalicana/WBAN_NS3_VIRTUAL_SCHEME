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
#include "ns3/point-to-point-star.h"
#include "ns3/error-model.h"
#include "DoctorRegisterApp.h"
#include "GatewayApp.h"
#include "PhoneApp.h"
#include "SensorApp.h"

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
void bro(Ptr<SensorApp> sens,Ptr<DoctorRegisterApp> doc)
{
  byte *buff = sens->GetSNj();
  string Kssk="";
  for(int q=0;q<16;q++)Kssk+=(byte)buff[q];
  doc->SetKssk(Kssk);
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
  LogComponentEnable("SensorApp",LOG_LEVEL_ALL);


  Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (.05));
   Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));
 
   Config::SetDefault ("ns3::BurstErrorModel::ErrorRate", DoubleValue (0.01));
   Config::SetDefault ("ns3::BurstErrorModel::BurstSize", StringValue ("ns3::UniformRandomVariable[Min=1|Max=3]"));
 
   Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (210));
   Config::SetDefault ("ns3::OnOffApplication::DataRate", DataRateValue (DataRate ("448kb/s")));
 
   std::string errorModelType = "ns3::RateErrorModel";

  int doctorNodeCnt=5;
  int gatewayNodeCnt=1;
  int patientNodeCnt=3;
  int perSensorNode = 15;
  int sensorNodeCnt=patientNodeCnt*perSensorNode;
  int totalNodes = doctorNodeCnt+gatewayNodeCnt+sensorNodeCnt+patientNodeCnt;

  NodeContainer GatewayNode;
  NodeContainer DoctorNodes;
  NodeContainer PatientNodes;
  NodeContainer SensorNodes;
  NodeContainer nodes;

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  PointToPointStarHelper p2pStar(totalNodes-1,pointToPoint);



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

  Ptr<Socket> sensor_listener[sensorNodeCnt];
  Ptr<Socket> sensor_speaker[sensorNodeCnt];

  int i=0;
  for(int j=0;j<doctorNodeCnt;i++,j++)
  {
    doctor_listener[j] = Socket::CreateSocket ( p2pStar.GetSpokeNode(i),TcpSocketFactory::GetTypeId ());
    doctor_speaker[j]  = Socket::CreateSocket ( p2pStar.GetSpokeNode(i),TcpSocketFactory::GetTypeId ());
  }
  for(int j=0;j<patientNodeCnt;i++,j++)
  {
    patient_listener[j] = Socket::CreateSocket ( p2pStar.GetSpokeNode(i),TcpSocketFactory::GetTypeId ());
    patient_speaker[j]  = Socket::CreateSocket ( p2pStar.GetSpokeNode(i),TcpSocketFactory::GetTypeId ());
  }
  for(int j=0;j<sensorNodeCnt;j++,i++)
  {
    sensor_listener[j] = Socket::CreateSocket ( p2pStar.GetSpokeNode(i),TcpSocketFactory::GetTypeId ());
    sensor_speaker[j]  = Socket::CreateSocket ( p2pStar.GetSpokeNode(i),TcpSocketFactory::GetTypeId ());
  }
  Ptr<Socket> gw_listener = Socket::CreateSocket (p2pStar.GetHub(), TcpSocketFactory::GetTypeId ());
  Ptr<Socket> gw_speaker = Socket::CreateSocket (p2pStar.GetHub(), TcpSocketFactory::GetTypeId ());

  Address doctorAddress[doctorNodeCnt];
  Address patientAddress[patientNodeCnt];
  Address gwAddress[totalNodes-1];
  Address sensorAddress[sensorNodeCnt];

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
  for(int j=0;j<sensorNodeCnt;i++,j++)
  {
    sensorAddress[j] =InetSocketAddress (p2pStar.GetSpokeIpv4Address (i), sinkPort);
  }

  Ptr<DoctorRegisterApp> doctor_app[doctorNodeCnt];
  Ptr<PhoneApp> phone_app[patientNodeCnt];
  Ptr<SensorApp> sensor_app[sensorNodeCnt];
  Ptr<GatewayApp> gw_app = CreateObject<GatewayApp> ();

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
    phone_app[j]->SetStopTime (Seconds (20.));
    phone_app[j]->Setup (patient_speaker[j],patient_listener[j], gwAddress[i]  , 1040, 1000, DataRate ("1Mbps"),sinkPort,PATIENT);
    (p2pStar.GetSpokeNode(i))->AddApplication(phone_app[j]);
  }
  for(int j=0;j<sensorNodeCnt;j++,i++)
  {
    sensor_app[j] = CreateObject<SensorApp> ();
    sensor_app[j]->SetStartTime (Seconds (1.));
    sensor_app[j]->SetStopTime (Seconds (20.));
    sensor_app[j]->Setup (sensor_speaker[j],sensor_listener[j], gwAddress[i]  , 1040, 1000, DataRate ("1Mbps"),sinkPort,SENSOR);
    (p2pStar.GetSpokeNode(i))->AddApplication(sensor_app[j]);
  }

  
  gw_app->SetStartTime (Seconds (1.));
  gw_app->SetStopTime (Seconds (20.));
  gw_app->Setup (gw_speaker,gw_listener, p2pStar.GetHubIpv4Address(0) , 1040, 1000, DataRate ("1Mbps"),sinkPort,GATEWAY,10);
  (p2pStar.GetHub())->AddApplication(gw_app);
  //ESTABLISH CONNECTION BETWEEN PHONE AND SENSORS


  i=doctorNodeCnt;
  //doctorNodeCnt+patientNodeCnt is where sensors starts
  int bs = doctorNodeCnt+patientNodeCnt;
  std::vector<NodeContainer> nodeAdjacencyList (perSensorNode*patientNodeCnt);
  
  for(int j=0;j<patientNodeCnt;j++,i++)
  {
    for(int k=0;k<perSensorNode;k++)
    {
      nodeAdjacencyList[j*perSensorNode+k] = NodeContainer (p2pStar.GetSpokeNode(i), p2pStar.GetSpokeNode(bs + j*perSensorNode + k));
    }
  }
  p2pStar.GetHub()->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
  std::vector<NetDeviceContainer> deviceAdjacencyList (perSensorNode*patientNodeCnt);
  ObjectFactory factory;
   factory.SetTypeId (errorModelType);
   Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
  for(uint32_t i=0; i<deviceAdjacencyList.size (); ++i)
  {
    deviceAdjacencyList[i] = p2p.Install (nodeAdjacencyList[i]);
    deviceAdjacencyList[i].Get(0)->SetAttribute("ReceiveErrorModel", PointerValue (em));
  }
  
  Ipv4AddressHelper ipv4;
  std::vector<Ipv4InterfaceContainer> interfaceAdjacencyList (perSensorNode*patientNodeCnt);
  for(uint32_t i=0; i<interfaceAdjacencyList.size (); ++i)
  {
      std::ostringstream subnet;
      subnet<<"11.1."<<i+1<<".0";
      ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
      interfaceAdjacencyList[i] = ipv4.Assign (deviceAdjacencyList[i]);
  }
  for(int j=0;j<patientNodeCnt;j++,i++)
  {
    for(int k=0;k<perSensorNode;k++)
    {
      //InetSocketAddress ad = InetSocketAddress::ConvertFrom();
      Address add = InetSocketAddress(interfaceAdjacencyList[j*perSensorNode+k].GetAddress(1),sinkPort);
      sensorAddress[j*perSensorNode+k] = add;
      //nodeAdjacencyList[j*3+k] = NodeContainer (p2pStar.GetSpokeNode(i), p2pStar.GetSpokeNode(bs + j*3 + k));
    }
  }


  //ESTABLISH CONNECTION BETWEEN SENSORS AND MEDICAL EXPERT
  i=doctorNodeCnt;
  //doctorNodeCnt+patientNodeCnt is where sensors starts
  bs = doctorNodeCnt+patientNodeCnt;
  std::vector<NodeContainer> nodeAdjacencyList2 (sensorNodeCnt*doctorNodeCnt);
  
  for(int j=0;j<doctorNodeCnt;j++)
  {
    for(int k=0;k<sensorNodeCnt;k++)
    {
      nodeAdjacencyList2[j*sensorNodeCnt+k] = NodeContainer (p2pStar.GetSpokeNode(j), p2pStar.GetSpokeNode(bs + k));
    }
  }

  PointToPointHelper p2p2;
  p2p2.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p2.SetChannelAttribute ("Delay", StringValue ("7ms"));
  //Doctor and sensors
  std::vector<NetDeviceContainer> deviceAdjacencyList2 (sensorNodeCnt*doctorNodeCnt);

  for(i=0; i<deviceAdjacencyList2.size (); ++i)
  {
    deviceAdjacencyList2[i] = p2p2.Install (nodeAdjacencyList2[i]);

  }

  Ipv4AddressHelper ipv42;
  std::vector<Ipv4InterfaceContainer> interfaceAdjacencyList2(sensorNodeCnt*doctorNodeCnt);
  for(uint32_t i=0; i<interfaceAdjacencyList2.size (); ++i)
  {
      std::ostringstream subnet;
      subnet<<"12.1."<<i+1<<".0";
      ipv42.SetBase (subnet.str ().c_str (), "255.255.255.0");
      interfaceAdjacencyList2[i] = ipv42.Assign (deviceAdjacencyList2[i]);
  }

  for(int j=0;j<doctorNodeCnt;j++)
  {
    for(int k=0;k<sensorNodeCnt;k++)
    {
      Address add = InetSocketAddress(interfaceAdjacencyList2[j*sensorNodeCnt +k].GetAddress(0),sinkPort);
      char x = j + (int)'0' + 1;
      string y = "";
      y+=x;
      y+='0';
      y+='0';
      y+='0';
      doctor_app[j]->SetMid(y);
      sensor_app[k]->AddAddress(y,add);
    }
  }
  AsciiTraceHelper ascii;
  pointToPoint.EnableAsciiAll(ascii.CreateFileStream("testing1.tr"));
  p2p.EnableAsciiAll(ascii.CreateFileStream("testing2.tr"));
  p2p2.EnableAsciiAll(ascii.CreateFileStream("testing3.tr"));



  //patient sends request to be registered 
  //for each patient randomly pick an ID ui and perform SHA256 with the user ID of the gateway send it to the patient.
  //doctor_app[0]->StartSending("591022132\nabcdefgh");
  Simulator::Schedule (Seconds (3), &DoctorRegisterApp::SendRegisterInfo, doctor_app[0], "1000" , "abcde", gwAddress[0]);
  Simulator::Schedule (Seconds (3.1), &DoctorRegisterApp::SendRegisterInfo, doctor_app[1], "2000" , "abcde", gwAddress[1]);
  Simulator::Schedule (Seconds (3.2), &DoctorRegisterApp::SendRegisterInfo, doctor_app[2], "3000" , "abcde", gwAddress[2]);
  Simulator::Schedule (Seconds (3.3), &DoctorRegisterApp::SendRegisterInfo, doctor_app[3], "4000" , "abcde", gwAddress[3]);
  Simulator::Schedule (Seconds (3.4), &DoctorRegisterApp::SendRegisterInfo, doctor_app[4], "5000" , "abcde", gwAddress[4]);

 


  Simulator::Schedule (Seconds (6), &PhoneApp::SendPing, phone_app[0]);
  Simulator::Schedule (Seconds (6.001), &PhoneApp::SendPing, phone_app[1]);
  Simulator::Schedule (Seconds (6.002), &PhoneApp::SendPing, phone_app[2]);

  byte *buff;
  for(i=0;i<patientNodeCnt;i++)
  {
    for(int j=0;j<perSensorNode;j++)
    {
      //sensor node will ping the server with UI of the patient
      //server will handle it
      Simulator::Schedule (Seconds (7.0 + (double)(j*0.0025) + (double)(i*0.025)), &init, sensor_app[i*perSensorNode+j], phone_app[i],sensorAddress[i*perSensorNode+j]);
    }
  }
  

  for(int i=0;i<3;i++)
  {
   // Simulator::Schedule (Seconds (9.0), &bro, sensor_app[i], doctor_app[i]);
    string g;

    Simulator::Schedule (Seconds (9.), &DoctorRegisterApp::SendLoginInfo, doctor_app[0], "1000" , "abcde", gwAddress[0], phone_app[0], sensor_app[i]);
     Simulator::Schedule (Seconds (9.), &DoctorRegisterApp::SendLoginInfo, doctor_app[0], "1000" , "abcde", gwAddress[1], phone_app[1], sensor_app[15+i]);
     Simulator::Schedule (Seconds (9.), &DoctorRegisterApp::SendLoginInfo, doctor_app[0], "1000" , "abcde", gwAddress[2], phone_app[2], sensor_app[30+i]);

      Simulator::Schedule (Seconds (9.), &DoctorRegisterApp::SendLoginInfo, doctor_app[1], "2000" , "abcde", gwAddress[0], phone_app[0], sensor_app[i]);
     Simulator::Schedule (Seconds (9.), &DoctorRegisterApp::SendLoginInfo, doctor_app[1], "2000" , "abcde", gwAddress[1], phone_app[1], sensor_app[15+i]);
     Simulator::Schedule (Seconds (9.), &DoctorRegisterApp::SendLoginInfo, doctor_app[1], "2000" , "abcde", gwAddress[2], phone_app[2], sensor_app[30+i]);

      Simulator::Schedule (Seconds (9.), &DoctorRegisterApp::SendLoginInfo, doctor_app[2], "3000" , "abcde", gwAddress[0], phone_app[0], sensor_app[i]);
     Simulator::Schedule (Seconds (9.), &DoctorRegisterApp::SendLoginInfo, doctor_app[2], "3000" , "abcde", gwAddress[1], phone_app[1], sensor_app[15+i]);
     Simulator::Schedule (Seconds (9.), &DoctorRegisterApp::SendLoginInfo, doctor_app[2], "3000" , "abcde", gwAddress[2], phone_app[2], sensor_app[30+i]);
  }






  
  Simulator::Stop (Seconds (20));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
