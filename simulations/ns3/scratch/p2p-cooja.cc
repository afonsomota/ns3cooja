#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/fd-net-device-module.h"
#include "ns3/config-store-module.h"
#include "ns3/point-to-point-module.h"

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
  uint32_t noCooja = 0;
  std::string coojaConfig("ns3CoojaWifi");
  int seed = 1;

  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::RealtimeSimulatorImpl::SynchronizationMode", StringValue ("HardLimit"));
  Config::SetDefault ("ns3::RealtimeSimulatorImpl::HardLimit", TimeValue (Time ("500ms")));
  
  CommandLine cmd;
  cmd.AddValue ("interPacketInterval","Inter packet interval [ms]",interPacketInterval);
  cmd.AddValue ("noClients","Number of clients",noClients);
  cmd.AddValue ("noCooja","Number of clients connected to coojs",noCooja);
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
  
 
  NodeContainer all (remote,clients);
  
  InternetStackHelper inetv6;
  inetv6.SetIpv4StackInstall (false);
  inetv6.Install (all);

  CoojaFdNetDevicesHelper coojaHelper (coojaConfig.c_str());

  //Create P2P Connections

  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));

  Ipv6StaticRoutingHelper ipv6RoutingHelper;
  Ptr<Ipv6StaticRouting> remoteHostStaticRouting = ipv6RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv6> ());


  NodeContainer coojaNodes;
  coojaNodes.Add (remote);

  uint32_t c = 1;

  for (int i = 0; i < noClients; i++)
    {
      Ptr<Node> cli = clients.Get (i);

      NetDeviceContainer devs = p2ph.Install (remoteHost, cli);

      std::stringstream ipv6AddrStream;
      ipv6AddrStream << "7777:" << i+1 << "::";
      Ipv6Address p2pNetwork (ipv6AddrStream.str().c_str());
      Ipv6AddressHelper ipv6;
      ipv6.SetBase (p2pNetwork, Ipv6Prefix (64));
      Ipv6InterfaceContainer ifs6 = ipv6.Assign (devs);

      ifs6.SetForwarding(0,true);
      ifs6.SetForwarding(1,true);
      ifs6.SetDefaultRouteInAllNodes(0);

      if (c > noCooja) continue;
      coojaNodes.Add (cli);
      uint32_t remoteIf = remoteHost->GetObject<Ipv6> ()->GetInterfaceForPrefix (p2pNetwork, Ipv6Prefix (64));
      Ipv6Address coojaGwAddr = cli->GetObject<Ipv6> ()->GetAddress (1,0).GetAddress ();
      Ipv6Address coojaNetwork = coojaHelper.GetNetwork (c);
      Ipv6Prefix coojaPrefix = coojaHelper.GetPrefix (c);
      remoteHostStaticRouting->AddNetworkRouteTo (coojaNetwork, coojaPrefix, coojaGwAddr, remoteIf);
      c++;
    }

  
  if (noCooja > 0) coojaHelper.CreateGatewayNodes (coojaNodes);
  
  //Background Traffic
  
  uint32_t dlPort = 1234;
  uint32_t ulPort = 2000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  
  for (uint32_t i = noCooja; i < clients.GetN(); i++)
      {
        ++ulPort;
        PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), dlPort));
        PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), ulPort));
  
        serverApps.Add (dlPacketSinkHelper.Install (clients.Get (i)));
  
        serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
  
//        UdpClientHelper ulClient (remoteHost->GetObject<Ipv6> ()->GetAddress (1,1).GetAddress (), ulPort);
        UdpClientHelper ulClient (Ipv6Address ("fd33::233:33:33:33"), ulPort);
        ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
        ulClient.SetAttribute ("MaxPackets", UintegerValue (UINT_MAX));
        ulClient.Install (clients.Get (i)).Start (MilliSeconds (120000 +rand->GetValue(0,interPacketInterval)));
       
      }
    
  
    serverApps.Start (Seconds (2));
  
  
    Simulator::Stop (Seconds (1200));
    Simulator::Run ();
  
}

