#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/fd-net-device-module.h"
#include "ns3/config-store-module.h"

#include "signal.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiCooja");

void
signalHandler (int signo)
{
  Simulator::Stop ();
  std::cout.flush ();
  std::cerr.flush ();
  exit (0);
}

void rxCallback (Ptr< const Packet > packet, const Address &address)
{
  Ipv6Header header;
  int s = packet->PeekHeader (header);
  std::cout <<s << " " << header.GetSourceAddress () << " " << address << "\n";
}


int main (int argc, char **argv){

  unsigned int interPacketInterval = 1000;
  int noClients = 50;
  std::string coojaConfig("ns3CoojaWifi");
  int seed = 1;

  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::RealtimeSimulatorImpl::SynchronizationMode", StringValue ("HardLimit"));
  Config::SetDefault ("ns3::RealtimeSimulatorImpl::HardLimit", TimeValue (Time ("10ms")));
  
  CommandLine cmd;
  cmd.AddValue ("interPacketInterval","Inter packet interval [ms]",interPacketInterval);
  cmd.AddValue ("noClients","Number of wifi stations per AP",noClients);
  cmd.AddValue ("CoojaConfig","Cooja integration configuration file",coojaConfig);
  cmd.AddValue ("seed","Random seed",seed);
  cmd.Parse (argc, argv);
  
  RngSeedManager::SetSeed (seed);

  signal (SIGINT, signalHandler);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  
  
  NodeContainer clients;
  NodeContainer remote;
  
  clients.Create (noClients);
  
  remote.Create (1);
  Ptr<Node> remoteHost = remote.Get (0);
  
 
  NodeContainer all (clients,remote);
  
  InternetStackHelper inetv6;
  inetv6.SetIpv4StackInstall (false);
  inetv6.Install (all);

  CoojaFdNetDevicesHelper coojaHelper (coojaConfig.c_str());
  
  //Create CSMA LAN
  
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer csmaDevs = csma.Install (all);
  
  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("7777:f00d::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer csmaIfs = ipv6.Assign (csmaDevs);
  //csmaIfs.SetForwarding (0, true);
  for (uint32_t i = 0; i < csmaIfs.GetN (); i++)
    {
      csmaIfs.SetForwarding (i, true);
    }
  
  csmaIfs.SetDefaultRouteInAllNodes (0);
  
  Ipv6StaticRoutingHelper ipv6RoutingHelper;
  Ptr<Ipv6StaticRouting> remoteHostStaticRouting = ipv6RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv6> ());
  uint32_t remoteIf = remoteHost->GetObject<Ipv6> ()->GetInterfaceForPrefix (Ipv6Address ("7777:f00d::"), Ipv6Prefix (64));
  
  
  uint32_t c = 0;
  
  for (int i = 0; i < noClients; i++)
    {
      if (c >= coojaHelper.GetNConnections()) break; 
      std::stringstream ipv6AddrStream;
      Ptr<Node> cli = clients.Get (i);
      Ipv6Address coojaGwAddr = cli->GetObject<Ipv6> ()->GetAddress (1,1).GetAddress ();
      Ipv6Address coojaNetwork = coojaHelper.GetNetwork (c);
      Ipv6Prefix coojaPrefix = coojaHelper.GetPrefix (c);
      remoteHostStaticRouting->AddNetworkRouteTo (coojaNetwork, coojaPrefix, coojaGwAddr, remoteIf);
    }

  coojaHelper.CreateGatewayNodes (all);
  
  //Background Traffic
  
  uint32_t dlPort = 1234;
  uint32_t ulPort = 2000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  
  for (uint32_t i = 0; i < clients.GetN(); i++)
      {
        ++ulPort;
        PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), dlPort));
        PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), ulPort));
  
        serverApps.Add (dlPacketSinkHelper.Install (clients.Get (i)));
  
        serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
  
        UdpClientHelper dlClient (clients.Get (i)->GetObject<Ipv6> ()->GetAddress (1,1).GetAddress (), dlPort);
        dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
        dlClient.SetAttribute ("MaxPackets", UintegerValue (UINT_MAX));
//        dlClient.Install (remoteHost).Start (MilliSeconds (2000 + i*5));
  
        UdpClientHelper ulClient (remoteHost->GetObject<Ipv6> ()->GetAddress (1,1).GetAddress (), ulPort);
        ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
        ulClient.SetAttribute ("MaxPackets", UintegerValue (UINT_MAX));
//        ulClient.Install (clients.Get (i)).Start (MilliSeconds (2000 + i*5));
       
//        clientApps.Add (dlClient.Install (remoteHost));
//        clientApps.Add (ulClient.Install (stas.Get (i)));
      }
    
  
    serverApps.Start (Seconds (2));
    clientApps.Start (Seconds (2));
  
    csma.EnablePcapAll("csma-cooja");
  
    Simulator::Stop (Seconds (1200));
    Simulator::Run ();
  
}

