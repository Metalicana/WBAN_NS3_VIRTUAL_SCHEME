
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
#include "GatewayApp.h"

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



namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("GatewayApp");
  NS_OBJECT_ENSURE_REGISTERED(GatewayApp);
  GatewayApp::GatewayApp ()
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

GatewayApp::~GatewayApp()
{
  speaker_socket = 0;
  listener_socket = 0;
}

void
GatewayApp::Setup (Ptr<Socket> _speaker_socket, Ptr<Socket> _listener_socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate,uint16_t _port, uint8_t _m_id)
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
GatewayApp::StartApplication (void)
{
  
  m_running = true;
  m_packetsSent = 0;
  InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
    if (listener_socket->Bind(local) == -1)
    {
      NS_FATAL_ERROR("Failed to bind socket");
    }
    NS_LOG_INFO("App started successfully");
  listener_socket->Listen();
  listener_socket->ShutdownSend();
  listener_socket->SetRecvCallback(MakeCallback(&GatewayApp::RecvString, this));
  listener_socket->SetAcceptCallback (
     MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
     MakeCallback (&GatewayApp::HandleAccept, this));
}
Ptr<Socket>
 GatewayApp::GetListeningSocket (void) const
 {
   NS_LOG_FUNCTION (this);
   return listener_socket;
 }
 
void 
GatewayApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

 if (listener_socket)
    {
      listener_socket->Close ();
    }
    if(speaker_socket)
    {
        speaker_socket->Close();
    }
}
void GatewayApp::DoctorRegistration(string Mid, string PW, Address address)
{
  //generate Kj and Kl ans Skey
  //Encrypt hash and stuff
  //produce output. send it to doctor

}
void
GatewayApp::HandleRead(Ptr<Socket> socket)
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
GatewayApp::SendPacket ()
{
  
  Ptr<Packet> packet = Create<Packet>();
  packet = packetize("This is a test message please print me");

  speaker_socket->Bind ();
  speaker_socket->Connect(m_peer);
  NS_LOG_INFO(speaker_socket->Send (packet));
  
  NS_LOG_INFO("Successfully sent data");
}
void GatewayApp::
RecvString(Ptr<Socket> sock)//Callback
{
   
    Address from;
    Ptr<Packet> packet = sock->RecvFrom (from);
    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();
    InetSocketAddress address = InetSocketAddress::ConvertFrom (from);

    // uint8_t data[sizeof(packet)];
    uint8_t data[255];
    packet->CopyData(data,sizeof(data));//Write the data in the package into data
    cout <<sock->GetNode()->GetId()<<" "<<"receive : '" << data <<"' from "<<address.GetIpv4 ()<< endl; 
    string Mid="",PW="";
    int mid;
    if(data[0]>='0' && data[0]=='9')//first character is identifier
    {
      mid = data[0]-'0';
      if(mid==DOCTOR_REGISTER)
      {
        //retrieve the Mid and PW separated by \n
        //parse it to a DoctorRegistration
        uint32_t i=0;
        while(data[i]=='\n')
        {
          Mid+=data[i];
          i++;
        }
        while(i<sizeof(data))
        {
          PW+=data[i];
          i++;
        }
        DoctorRegistration(Mid,PW,address);
      }
    }
    char a[sizeof(data)];
    for(uint32_t i=0;i<sizeof(data);i++){
        a[i]=data[i];
    }
    string strres = string(a);
    cout<<"The received string is "<<strres<<endl;


 
}

void GatewayApp::HandleAccept (Ptr<Socket> s, const Address& from)
 {
   s->SetRecvCallback (MakeCallback (&GatewayApp::RecvString, this));
   m_socketList.push_back (s);
 }
void 
GatewayApp::ScheduleTx (void)
{
  
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ()))+ Seconds(2));
      m_sendEvent = Simulator::Schedule (tNext, &GatewayApp::SendPacket, this);
    }
}
void GatewayApp::StartSending()
{
  m_sendEvent = Simulator::Schedule (Seconds(1.01), &GatewayApp::SendPacket,this);
}
Ptr<Packet> GatewayApp:: packetize (string str)
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