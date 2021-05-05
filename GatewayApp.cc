
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
GatewayApp::Setup (Ptr<Socket> _speaker_socket, Ptr<Socket> _listener_socket, 
Address address, uint32_t packetSize, uint32_t nPackets, 
DataRate dataRate,uint16_t _port, uint8_t _m_id, uint32_t _ID)
{
  listener_socket = _listener_socket;
  speaker_socket = _speaker_socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
  port = _port;
  m_id = _m_id;
  ID = _ID;
}
void GatewayApp::ReadKeys()
{
  fstream input;
  input.open("DoctorKeys",fstream::in);
  string g;
  int l =0,x,p=0;
  byte *current_key[6];
  key_comb cur;
  while(std::getline(input,g))
  {
    
    stringstream ss(g);
    
    p=0;
    while(ss >> x)
    {
      current_key[l][p]=x;
      p++;
    }
    l++;
    if(l==6)
    {
      l=0;
      cur.Kj = SecByteBlock(current_key[0], AES::DEFAULT_KEYLENGTH);
      cur.Kl = SecByteBlock(current_key[1], AES::DEFAULT_KEYLENGTH);
      cur.Skey = SecByteBlock(current_key[2], AES::DEFAULT_KEYLENGTH);
      cur.KjBlock = SecByteBlock(current_key[3], AES::BLOCKSIZE);
      cur.KlBlock = SecByteBlock(current_key[4], AES::BLOCKSIZE);
      cur.SkeyBlock = SecByteBlock(current_key[5], AES::BLOCKSIZE);
      keys.push_back(cur);
    }
  }
  input.close();
}
void GatewayApp::WriteKeys(key_comb cur)
{
  fstream output,input;
  input.open("/home/radivm/Desktop/ns-allinone-3.33/ns-3.33/scratch/wban_final/DoctorKeys",fstream::in);
  
  string g;
  string k="";
  while(getline(input,g))
  {
    k+=g;
    k+='\n';
  }
  input.close();
  output.open("/home/radivm/Desktop/ns-allinone-3.33/ns-3.33/scratch/wban_final/DoctorKeys",fstream::out);
  output<<k;
  byte *h[6];
  h[0] = cur.Kj.data();
  h[1] = cur.Kl.data();
  h[2] = cur.Skey.data();
  h[3] = cur.KjBlock.data();
  h[4] = cur.KlBlock.data();
  h[5] = cur.SkeyBlock.data();
  for(int q=0;q<6;q++)
  {
    int j;
    if(q<3)j=AES::DEFAULT_KEYLENGTH;
    else j = AES::BLOCKSIZE;
    for(int i=0; i < j ; i++)
    {
      output<<(int)h[q][i] << " ";
    }
     output<<endl;
  }
  output.close();
  
}
void GatewayApp::WriteIndex(string Mid)
{
  fstream output,input;

  input.open("/home/radivm/Desktop/ns-allinone-3.33/ns-3.33/scratch/wban_final/DoctorIndex",fstream::in);
  
  string g;
  string h="";
  while(input>>g)
  {
    h+=g;
    h+='\n';
  }
  input.close();
  h+=Mid;
  output.open("/home/radivm/Desktop/ns-allinone-3.33/ns-3.33/scratch/wban_final/DoctorIndex",fstream::out);
  output<<h<<endl;
  output.close();
  
  
}
void GatewayApp::ReadDoctorIndices()
{
  fstream input;
  input.open("DoctorIndex",fstream::in);
  string g;
  while(std::getline(input,g))
  {
    doctors_index[g] = registered_doctor++;
  }
  input.close();
}
uint32_t GatewayApp:: GetID()
{
  return ID;
}
void
GatewayApp::StartApplication (void)
{
  
  m_running = true;
  m_packetsSent = 0;
  registered_doctor=0;
  //read from file the three keys.
  ReadKeys();
  ReadDoctorIndices();
  //I read three strings.
  //they are my kj kl and Skey
  //I create a key_comb object with it
  //I enter it in my keys vector
  //I scan another file
  //I take all the string Mid, their order is their index
  InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
    if (listener_socket->Bind(local) == -1)
    {
      NS_FATAL_ERROR("Failed to bind socket");
    }
    NS_LOG_INFO(TEAL_CODE<<"Server started successfully"<<END_CODE);
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
  for(auto x : m_socketList)
  {
    x->Close();
  }
  m_socketList.clear();
}
void GatewayApp::DoctorRegistration(string Mid, string PW, Address address)
{
  //generate Kj and Kl ans Skey
  //Encrypt hash and stuff
  //produce output. send it to doctor
  //save the three keys of the medical professional map it with his Mid
  AutoSeededRandomPool rnd;

// Generate a random key
  SecByteBlock Kj(0x00, AES::DEFAULT_KEYLENGTH);
  SecByteBlock Kl(0x00, AES::DEFAULT_KEYLENGTH);
  SecByteBlock Skey(0x00, AES::DEFAULT_KEYLENGTH);
  SecByteBlock KjBlock(AES::BLOCKSIZE);
  SecByteBlock KlBlock(AES::BLOCKSIZE);
  SecByteBlock SkeyBlock(AES::BLOCKSIZE);

  rnd.GenerateBlock( Kj, Kj.size() );
  rnd.GenerateBlock( Kl, Kl.size() );
  rnd.GenerateBlock( Skey, Skey.size() );

  rnd.GenerateBlock( KjBlock, KjBlock.size() );
  rnd.GenerateBlock( KlBlock, KlBlock.size() );
  rnd.GenerateBlock( SkeyBlock, SkeyBlock.size() );

  WriteIndex(Mid);
  key_comb cur;
  cur.Kj = SecByteBlock(Kj.data(), AES::DEFAULT_KEYLENGTH);
  cur.Kl = SecByteBlock(Kl.data(), AES::DEFAULT_KEYLENGTH);
  cur.Skey = SecByteBlock(Skey.data(), AES::DEFAULT_KEYLENGTH);
  cur.KjBlock = SecByteBlock(KjBlock.data(), AES::BLOCKSIZE);
  cur.KlBlock = SecByteBlock(KlBlock.data(), AES::BLOCKSIZE);
  cur.SkeyBlock = SecByteBlock(SkeyBlock.data(), AES::BLOCKSIZE);

  WriteKeys(cur);
  
  doctors_index[Mid] = registered_doctor++;

  keys.push_back(cur);
  //here we do the encryption and stuff
  //we concat the ID string 
  string IDgw = "";
  uint32_t temp = ID;
  if(temp == 0)IDgw+='0';
  while(temp!=0)
  {
    IDgw+=(char)(temp%10+'0');
    temp/=10;
  }
  reverse(IDgw.begin(),IDgw.end());
  string cc = Mid + IDgw;
  byte plainText[(int)cc.size()+1];
  for(int i=0;i<(int)cc.size();i++)
  {
    plainText[i] = (byte)cc[i];
  }
  plainText[cc.size()]=0;
  CFB_Mode<AES>::Encryption cfbEncryption(Kj, Kj.size(), KjBlock);
  cfbEncryption.ProcessData(plainText, plainText, (size_t)((int)cc.size()+1) );

  HexEncoder encoder(new FileSink(std::cout));

  std::string digest1,digest2,digest3;

  SHA3_256 hash;
  hash.Update((const byte*)Mid.data(), Mid.size());
  digest1.resize(hash.DigestSize());
  hash.Final((byte*)&digest1[0]);

  hash.Update((const byte*)PW.data(), PW.size());
  digest2.resize(hash.DigestSize());
  hash.Final((byte*)&digest2[0]);

  hash.Update((const byte*)Skey.data(), Skey.size());
  digest3.resize(hash.DigestSize());
  hash.Final((byte*)&digest3[0]);

  byte hash_string[(int)digest1.size()];
  for(int i=0;i<(int)digest1.size();i++)
  {
    hash_string[i] = (byte)(digest1[i]^digest2[i]^digest3[i]);
  }

  //we need to send hash_string
  //we need to send  plainText

  
  uint8_t buff[3*AES::DEFAULT_KEYLENGTH + 2*AES::BLOCKSIZE + 32 + cc.size()+8];
  buff[0]=(byte)7;
  buff[1]=(byte)((int)cc.size());
  int j=2;
  int i;

  for(i=0;i<(int)cc.size();i++)buff[j++]=plainText[i];

  buff[j++]=(byte)32;
  NS_LOG_INFO(TEAL_CODE<<buff[j-1]<<END_CODE);
  for(i=0;i<32;i++)buff[j++]=hash_string[i];

  buff[j++]=(byte)AES::DEFAULT_KEYLENGTH;
  for(i=0;i<AES::DEFAULT_KEYLENGTH;i++)buff[j++]=Skey[i];

  buff[j++]=(byte)AES::DEFAULT_KEYLENGTH;
  for(i=0;i<AES::DEFAULT_KEYLENGTH;i++)buff[j++]=SkeyBlock[i];

  buff[j++]=(byte)AES::DEFAULT_KEYLENGTH;
  for(i=0;i<AES::DEFAULT_KEYLENGTH;i++)buff[j++]=Kj[i];

  buff[j++]=(byte)AES::BLOCKSIZE;
  for(i=0;i<AES::BLOCKSIZE;i++)buff[j++]=KjBlock[i];

  buff[j++]=(byte)AES::DEFAULT_KEYLENGTH;
  for(i=0;i<AES::DEFAULT_KEYLENGTH;i++)buff[j++]=Kl[i];

  buff[j++]=(byte)AES::BLOCKSIZE;
  for(i=0;i<AES::BLOCKSIZE;i++)buff[j++]=KlBlock[i];
  Ptr<Packet> packet = Create<Packet>(buff,j);
  
  SendPacket(packet, address);

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
GatewayApp::SendPacket (Ptr<Packet> packet, Address address)
{
  speaker_socket->Bind ();
  speaker_socket->Connect(address);
  
  speaker_socket->Send (packet);
  
  NS_LOG_INFO("Successfully sent data");
}
void GatewayApp::
RecvString(Ptr<Socket> sock)//Callback
{
   
    Address from;
    Ptr<Packet> packet;
    while(packet = sock->RecvFrom (from))
    {
      packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();
    InetSocketAddress address = InetSocketAddress::ConvertFrom (from);
    // uint8_t data[sizeof(packet)];
    byte data[255];
    packet->CopyData(data,sizeof(data));//Write the data in the package into data
    cout <<sock->GetNode()->GetId()<<" "<<"receive : '" << data <<"' from "<< address.GetIpv4 ()<< endl;
    Address ad = InetSocketAddress(address.GetIpv4(),port);
    string Mid="",PW="";
    int mid = (int)data[0] -(int)'0';
    if(mid>= 0 && mid<= 9 )//first character is identifier
    {
      if(mid==DOCTOR_REGISTER)
      {
        //retrieve the Mid and PW separated by \n
        //parse it to a DoctorRegistration
        NS_LOG_INFO(YELLOW_CODE<<"Incoming doctor register request" << END_CODE);
        uint32_t i=1;
        while(data[i]!=(int)'\n')
        {
          Mid+=(char)((int)data[i]);
          i++;
        }
        i++;
        while(data[i]!=0)
        {
          PW+=(char)((int)data[i]);
          i++;
        }
        NS_LOG_INFO(PURPLE_CODE<<"Mid: " << Mid << ", " << "PW: " << PW << END_CODE);
        DoctorRegistration(Mid,PW,ad);
      }
    }
    char a[sizeof(data)];
    for(uint32_t i=0;i<sizeof(data);i++){
        a[i]=data[i];
      }
      string strres = string(a);
    }
}

void GatewayApp::HandleAccept (Ptr<Socket> s, const Address& from)
 {
   
   s->SetRecvCallback (MakeCallback (&GatewayApp::RecvString, this));
   m_socketList.push_back (s);
 }
void 
GatewayApp::ScheduleTx (Ptr<Packet> packet, Address address)
{
  
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ()))+ Seconds(2));
      m_sendEvent = Simulator::Schedule (tNext, &GatewayApp::SendPacket,this,packet,address);
    }
}
void GatewayApp::StartSending()
{
  /*Ptr<Packet> packet = Create<Packet>();
  Address address("10.1.1.0");
  m_sendEvent = Simulator::Schedule (Seconds(1.01), &GatewayApp::SendPacket,this,packet,address);*/
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
