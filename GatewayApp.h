#ifndef NS3_GATEWAY_APP_H
#define NS3_GATEWAY_APP_H

#include <fstream>
#include <map>
#include <string>
#include <vector>
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


using namespace std;
using namespace ns3;
using namespace CryptoPP;
namespace ns3
{

struct key_comb
  {
    SecByteBlock Kj,Kl,Skey;
    SecByteBlock KjBlock, KlBlock, SkeyBlock;
  };
class GatewayApp : public Application 
{
    
public:

    
  GatewayApp ();
  virtual ~GatewayApp();
  void PatientRegister(Address add);
  void SendPacket (Ptr<Packet> packet,Address address);
  void StartSending();
  void RecvString(Ptr<Socket> sock);
  void Setup (Ptr<Socket> _speaker_socket, Ptr<Socket> _listener_socket, 
  Address address, uint32_t packetSize, uint32_t nPackets, 
  DataRate dataRate,uint16_t _port, uint8_t _m_id, uint32_t ID);
  uint32_t GetID();

private:
  void DoctorRegistration(string Mid, string PW, Address address);
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void HandleAccept (Ptr<Socket> s, const Address& from);
  void HandleRead(Ptr<Socket> socket);
  Ptr<Packet> packetize (string str);
  Ptr<Socket> GetListeningSocket (void) const;
  void ReadKeys();
  void ReadDoctorIndices();
  void WriteKeys(key_comb cur);
  void WriteIndex(string g);
  void SensorRegister(string peer, Address add);

  void ScheduleTx (Ptr<Packet> packet,Address address);
  
  
  Ptr<Socket>     speaker_socket;
  Ptr<Socket>     listener_socket;
  Address         m_peer;
  uint32_t        ID;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  uint32_t        registered_doctor;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
  uint16_t        port;
  std::list<Ptr<Socket> > m_socketList;
  vector <Ptr<Socket> > m_speakers;
  uint8_t         m_id;
  map<string,int> doctors_index;
  map<string, bool> doctor_exist;
  map<string, Address> peer_address;
  map<string, string> KGWU;
  vector<key_comb> keys; 
};
}

#endif
