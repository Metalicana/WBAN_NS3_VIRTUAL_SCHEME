
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

#include "SensorApp.h"

using namespace ns3;
using namespace std;

#define PURPLE_CODE "\033[95m"
#define CYAN_CODE "\033[96m"
#define TEAL_CODE "\033[36m"
#define BLUE_CODE "\033[94m"
#define GREEN_CODE "\033[32m"
#define YELLOW_CODE "\033[33m"
#define LIGHT_YELLOW_CODE "\033[93m"
#define RED_CODE "\033[91m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

#define DOCTOR_REGISTER 5
#define GATEWAY 6
#define PATIENT 7
#define SENSOR 8

namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("SensorApp");
  NS_OBJECT_ENSURE_REGISTERED(SensorApp);
  SensorApp::SensorApp ()
  : speaker_socket (0), 
    listener_socket (0), 
    m_peer (), 
    m_packetSize (0), 
    m_nPackets (0), 
    m_dataRate (0), 
    m_sendEvent (), 
    m_running (false), 
    m_packetsSent (0),
    port(0)
{
}

SensorApp::~SensorApp()
{
  speaker_socket = 0;
  listener_socket = 0;
}

void
SensorApp::Setup (Ptr<Socket> _speaker_socket, Ptr<Socket> _listener_socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate,uint16_t _port, uint8_t _m_id)
{
  listener_socket = _listener_socket;
  speaker_socket = _speaker_socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
  port = _port;
  m_id = _m_id;
}
void
SensorApp::SetBuff(byte *a)
{
  for(int i=0;i<16;i++)
  {
    buff[i]=a[i];
  }
}
void
SensorApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
  if (listener_socket->Bind(local) == -1)
  {
    NS_FATAL_ERROR("Failed to bind socket");
  }
  NS_LOG_INFO(RED_CODE << "OH NO IDK WHAT TO DO ANYMORE HAHA" << END_CODE);
  NS_LOG_INFO(RED_CODE<<"Sensor started successfully"<<END_CODE);
  listener_socket->Listen();
  listener_socket->ShutdownSend();
  listener_socket->SetRecvCallback(MakeCallback(&SensorApp::RecvString, this));
  listener_socket->SetAcceptCallback (
     MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
     MakeCallback (&SensorApp::HandleAccept, this));
}
Ptr<Socket>
 SensorApp::GetListeningSocket (void) const
 {
   NS_LOG_FUNCTION (this);
   return listener_socket;
 }
 
void 
SensorApp::StopApplication (void)
{
  m_running = false;

  if(m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if(listener_socket)
    {
      listener_socket->Close ();
    }
    if(speaker_socket)
    {
        speaker_socket->Close();
    }
  for(auto x : m_socketList)
  {
    x->Close();
  }
  m_socketList.clear();
}

void
SensorApp::HandleRead(Ptr<Socket> socket)
  {
    cout << "This was invoked\n";
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
      
      NS_LOG_INFO(TEAL_CODE << "HandleReadOne : Received a Packet of size: " << packet->GetSize() << " at time " << Now().GetSeconds() << END_CODE);
      NS_LOG_INFO(packet->ToString());
    }
  }
  
void 
SensorApp::SendPacket (Ptr<Packet> packet)
{
  
  speaker_socket->Bind ();
  speaker_socket->Connect(m_peer);
  speaker_socket->Send(packet);
  
  
  NS_LOG_INFO("Successfully sent data");
}
void SensorApp::SendRegisterInfo(string Mid, string PW, Address add)
{
  Ptr<Packet> p = packetize((char)DOCTOR_REGISTER + Mid + PW);
  speaker_socket->Bind ();
  speaker_socket->Connect(add);
  
  speaker_socket->Send (p);
  NS_LOG_INFO(BLUE_CODE << "Successfully sent Mid and PW to GW" << END_CODE);

}
void SensorApp::
RecvString(Ptr<Socket> sock)//Callback
{
   
    Address from;
    Ptr<Packet> packet;
    while( (packet = sock->RecvFrom (from)) )
    {
      if(packet->GetSize()==0)break;
      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();
      InetSocketAddress address = InetSocketAddress::ConvertFrom (from);
      NS_LOG_INFO(RED_CODE<<"Incoming information from Gateway for sensor"<<END_CODE);
    // uint8_t data[sizeof(packet)];
      uint8_t data[255];
      packet->CopyData(data,sizeof(data));//Write the data in the package into data
      //cout <<sock->GetNode()->GetId()<<" "<<"receive : '" << data <<"' from "<<address.GetIpv4 ()<< endl;  
      int num = (int)data[0];
      int prev;
      int i=1;
      for(int q=0;q<num;q++)
      {
        prev = (int)data[i];
        i++;
        cout << prev << ": ";
        for(int j=0;j<prev;i++,j++)
        {
          cout << (int)data[i] << " ";
        }
        cout << endl;
        
      }
    }
    
 
}
void SensorApp::SendPing(byte a[])
{
  byte buff[17];
  buff[0] = SENSOR + '0';
  for(int i=1,j=0;j<16;j++,i++)
  {
    buff[i]=a[j];
  }
  Ptr<Packet> p = Create<Packet>(buff,17);
  SendPacket(p);
}
void SensorApp::HandleAccept (Ptr<Socket> s, const Address& from)
 {
   s->SetRecvCallback (MakeCallback (&SensorApp::RecvString, this));
   m_socketList.push_back (s);
 }
void 
SensorApp::ScheduleTx (void)
{
  Ptr<Packet> packet = Create<Packet>();
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ()))+ Seconds(2));
      m_sendEvent = Simulator::Schedule (tNext, &SensorApp::SendPacket, this,packet);
    }
}
void SensorApp::StartSending(string g)
{
  Ptr<Packet>packet = packetize(g);
  m_sendEvent = Simulator::Schedule (Seconds(1.01), &SensorApp::SendPacket,this,packet);
}
Ptr<Packet> SensorApp:: packetize (string str)
{ 
// uint8_t buffer[sizeof(str)] ;//You can also create a package based on the string size
  uint8_t buffer[255] ;
  uint32_t len = str.length();
  for(uint32_t i=0;i<len;i++)
  {
    buffer[i]=str[i];//char and uint_8 are assigned one by one
  }
  buffer[len]='\0';
  Ptr<Packet> p = Create<Packet>(buffer,sizeof(buffer));//Write buffer to the package
  return p;
} 

}
