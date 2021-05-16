
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
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
#include "ns3/attribute.h"
#include "ns3/attribute-helper.h"

#include "cryptopp/aes.h"
#include "cryptopp/default.h"
#include "cryptopp/osrng.h"
#include "cryptopp/modes.h"
#include "cryptopp/cryptlib.h"
#include "cryptopp/seckey.h"
#include "cryptopp/randpool.h"
#include "cryptopp/rdrand.h"
#include "cryptopp/sha3.h"
#include "cryptopp/hex.h"
#include "cryptopp/files.h"

#include "DoctorRegisterApp.h"
#include "SensorApp.h"
#include "PhoneApp.h"

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

#define DOCTOR_LOGIN 4
#define DOCTOR_REGISTER 5
#define GATEWAY 6

namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("DoctorRegisterApp");
  NS_OBJECT_ENSURE_REGISTERED(DoctorRegisterApp);
  DoctorRegisterApp::DoctorRegisterApp ()
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

DoctorRegisterApp::~DoctorRegisterApp()
{
  speaker_socket = 0;
  listener_socket = 0;
}

void
DoctorRegisterApp::Setup (Ptr<Socket> _speaker_socket, Ptr<Socket> _listener_socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate,uint16_t _port, uint8_t _m_id)
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
DoctorRegisterApp::StartApplication (void)
{
  
  m_running = true;
  m_packetsSent = 0;
  InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
    if (listener_socket->Bind(local) == -1)
    {
      NS_FATAL_ERROR("Failed to bind socket");
    }
    NS_LOG_INFO(RED_CODE<<"Doctor app started successfully"<<END_CODE);
  listener_socket->Listen();
  listener_socket->ShutdownSend();
  listener_socket->SetRecvCallback(MakeCallback(&DoctorRegisterApp::RecvString, this));
  listener_socket->SetAcceptCallback (
     MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
     MakeCallback (&DoctorRegisterApp::HandleAccept, this));
}
Ptr<Socket>
 DoctorRegisterApp::GetListeningSocket (void) const
 {
   NS_LOG_FUNCTION (this);
   return listener_socket;
 }
 
void 
DoctorRegisterApp::StopApplication (void)
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
DoctorRegisterApp::HandleRead(Ptr<Socket> socket)
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
DoctorRegisterApp::SendPacket (Ptr<Packet> packet)
{
  
  speaker_socket->Bind ();
  speaker_socket->Connect(m_peer);
  speaker_socket->Send(packet);
  
  
  NS_LOG_INFO("Successfully sent data");
}
void DoctorRegisterApp::SendRegisterInfo(string Mid, string PW, Address add)
{
  char x = (char)(DOCTOR_REGISTER + '0');
  string g = x + Mid  + '\n' + PW;
  Ptr<Packet> p = packetize(g);

  Ptr<Socket> speak = Socket::CreateSocket (GetNode(), TcpSocketFactory::GetTypeId ());

  speak->Bind ();
  speak->Connect(add);
  
  speak->Send (p);
 // speaker_socket->ShutdownSend();
  NS_LOG_INFO(BLUE_CODE << "Successfully sent Mid and PW to GW" << END_CODE);

}
void DoctorRegisterApp::SendLoginInfo(string Mid, string PW, Address add, Ptr<PhoneApp> patient, Ptr<SensorApp> sensor)
{
  char x = (char)(DOCTOR_REGISTER + '0');
  string g = x + Mid  + '\n' + PW;

  string digest1,digest2,digest3;

  SHA3_256 hash;
  
  byte buff[32];
  for(int i=0;i<(int)Mid.size();i++)buff[i]=(byte)Mid[i];

  hash.Update(buff, (int)Mid.size());
  digest1.resize(hash.DigestSize());
  hash.Final((byte*)&digest1[0]);
  for(int i=0;i<(int)PW.size();i++)buff[i]=(byte)PW[i];

  hash.Update(buff, (int)PW.size());
  digest2.resize(hash.DigestSize());
  hash.Final((byte*)&digest2[0]);

  hash.Update(Skey, AES::BLOCKSIZE);
  digest3.resize(hash.DigestSize());
  hash.Final((byte*)&digest3[0]);

  byte hash_string[(int)digest1.size()];
  for(int i=0;i<(int)digest1.size();i++)
  {
    hash_string[i] = (byte)(digest1[i]^digest2[i]^digest3[i]);
  }
  for(int i=0;i<32;i++)
  {
    if((int)hash_string[i] != (int)Ni[i] )
    {
      NS_LOG_INFO(GREEN_CODE << "WRONG CREDENTIALS. LOGIN FAILED" << END_CODE);
      return;
    }
  }

  
  //digest1 is H(Mid)
  AutoSeededRandomPool rnd;
  SecByteBlock M(0x00, AES::DEFAULT_KEYLENGTH);
  rnd.GenerateBlock(M,M.size());
  byte *Mb = M.data();
  byte *Ui = patient->GetUi();
  byte *Nj = sensor->GetSNj();
  byte CIDi[10000];
  Time cur = Simulator::Now();
  TimeValue tv(cur);
  Ptr<AttributeChecker> check;
  g = tv.SerializeToString(check);

  SecByteBlock key(Kl, AES::DEFAULT_KEYLENGTH);
  SecByteBlock iv(KlIV,AES::BLOCKSIZE);
  int i=0;
  for(int j=0;j<32;j++,i++)CIDi[i]=(byte)digest1[j];
  for(int j=0;j<16;j++,i++)CIDi[i] = Mb[j];
  for(int j=0;j<16;j++,i++)CIDi[i] = Ui[j];
  for(int j=0;j<16;j++,i++)CIDi[i] = Nj[j];
  CIDi[i++]=g.size();
  for(int j=0;j<g.size();j++,i++)CIDi[i]=(byte)g[j];

  CFB_Mode<AES>::Encryption cfbEncryption(key, key.size(), iv);
  cfbEncryption.ProcessData(CIDi, CIDi, (size_t)(i+1) );

  byte message[10000];
  message[0] =DOCTOR_LOGIN+'0';
  message[1] = 3;
  message[2] = i;
  int sz = i;
  i=3;
  for(int j=0;j<sz;j++,i++)message[i]=CIDi[j];
  message[i++]=6;
  for(int j=0;j<6;j++,i++)message[i] = C[j];
  message[i++]=g.size();
  for(int j=0;j<(int)g.size();j++,i++)message[i]=(byte)g[j];
  
  Ptr<Packet> p = Create<Packet>(message,i);
  Ptr<Socket> speak = Socket::CreateSocket (GetNode(), TcpSocketFactory::GetTypeId ());
  speak->Bind();
  speak->Connect(m_peer);
  speak->Send(p);
  speak->ShutdownSend();
  NS_LOG_INFO(PURPLE_CODE << "Successfully sent Login info to GW" << END_CODE);


 /* Ptr<Packet> p = packetize(g);
  speaker_socket->Bind ();
  speaker_socket->Connect(add);
  
  speaker_socket->Send (p);
  speaker_socket->ShutdownSend();
  NS_LOG_INFO(BLUE_CODE << "Successfully sent Mid and PW to GW" << END_CODE);*/

}
void DoctorRegisterApp::
RecvString(Ptr<Socket> sock)//Callback
{
   
    Address from;
    Ptr<Packet> packet;
    while( (packet = sock->RecvFrom (from)) )
    {
      NS_LOG_INFO(YELLOW_CODE << "CODE RECEIVED AT " << Now().GetSeconds() << END_CODE);
      if(packet->GetSize()==0)break;
      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();
      InetSocketAddress address = InetSocketAddress::ConvertFrom (from);
      NS_LOG_INFO(RED_CODE<<"Incoming information from Gateway"<<END_CODE);
      //uint8_t data[sizeof(packet)];
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
          if(q==0)C[j]=data[i];
          else if(q==1)Ni[j]=data[i];
          else if(q==2)Skey[j]=data[i];
          else if(q==3)SkeyIV[j]=data[i];
          else if(q==4)Kj[j]=data[i];
          else if(q==5)KjIV[j]=data[i];
          else if(q==6)Kl[j]=data[i];
          else KlIV[j]=data[i];
          cout << (int)data[i] << " ";
        }
        cout << endl;
        
      }
    }
    
 
}

void DoctorRegisterApp::HandleAccept (Ptr<Socket> s, const Address& from)
 {
   s->SetRecvCallback (MakeCallback (&DoctorRegisterApp::RecvString, this));
   m_socketList.push_back (s);
 }
void 
DoctorRegisterApp::ScheduleTx (void)
{
  Ptr<Packet> packet = Create<Packet>();
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ()))+ Seconds(2));
      m_sendEvent = Simulator::Schedule (tNext, &DoctorRegisterApp::SendPacket, this,packet);
    }
}
void DoctorRegisterApp::StartSending(string g)
{
  Ptr<Packet>packet = packetize(g);
  m_sendEvent = Simulator::Schedule (Seconds(1.01), &DoctorRegisterApp::SendPacket,this,packet);
}
Ptr<Packet> DoctorRegisterApp:: packetize (string str)
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

