#ifndef NS3_SENSOR_APP_H
#define NS3_SENSOR_APP_H

#include <fstream>
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

using namespace std;
using namespace ns3;
using namespace CryptoPP;
namespace ns3
{
  class SensorApp : public Application 
{
    
public:

    
  SensorApp ();
  virtual ~SensorApp();
  void SendPing(byte a[]);
  void AddAddress(string g, Address a);
  void SendPacket (Ptr<Packet> packet);
  void StartSending(string g);
  void RecvString(Ptr<Socket> sock);
  void SendRegisterInfo(string Mid, string PW, Address add);
  void SetBuff(byte *a);
  void Setup (Ptr<Socket> _speaker_socket, Ptr<Socket> _listener_socket, Address address,
   uint32_t packetSize, uint32_t nPackets,
    DataRate dataRate,uint16_t _port, uint8_t _m_id);
  byte* GetUi();
  byte* GetSNj();
  byte* GetKUSNj();

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
  map<string,Address> doctor_address;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
  uint16_t        port;
  std::list<Ptr<Socket> > m_socketList; 
  uint8_t         m_id;
  byte            buff[32];
  byte            SNj[16];
  byte            KUSNj[32];
  byte            Ui[16];

};

}

#endif
