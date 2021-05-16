#ifndef NS3_DOCTOR_REGISTER_APP_H
#define NS3_DOCTOR_REGISTER_APP_H

#include <fstream>
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
#include "SensorApp.h"
#include "PhoneApp.h"

using namespace std;
using namespace ns3;
using namespace CryptoPP;
namespace ns3
{
  class DoctorRegisterApp : public Application 
{
    
public:

    
  DoctorRegisterApp ();
  virtual ~DoctorRegisterApp();
  void SetKssk(string g);
  void SetMid(string g);
  void SendPacket (Ptr<Packet> packet);
  void StartSending(string g);
  Address GetAddress(string g);
  void SetAddress(string g,Address add);
  void RecvString(Ptr<Socket> sock);
  void SendRegisterInfo(string Mid, string PW, Address add);
  void SendLoginInfo(string Mid, string PW, Address add,Ptr<PhoneApp> patient, Ptr<SensorApp> sensor);
  void Setup (Ptr<Socket> _speaker_socket, Ptr<Socket> _listener_socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate,uint16_t _port, uint8_t _m_id);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void HandleAccept (Ptr<Socket> s, const Address& from);
  void HandleRead(Ptr<Socket> socket);
  Ptr<Packet> packetize (string str);
  Ptr<Socket> GetListeningSocket (void) const;

  void ScheduleTx (void);
  

  Ptr<Socket>     speaker_socket;
  Ptr<Socket>     listener_socket;
  
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  string          Kssk;
  string          Mid;
  uint32_t        m_packetsSent;
  uint16_t        port;
  map<string,Address> doctor_address;
  std::list<Ptr<Socket> > m_socketList; 
  uint8_t         m_id;
  byte            Ni[32];
  byte            C[6];

  byte            Skey[16];
  byte            SkeyIV[16];
  byte            Kj[16];
  byte            KjIV[16];
  byte            Kl[16];
  byte            KlIV[16];
  byte            M[16];


  
};

}

#endif
