/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
//#include "ns3/ipv6.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/fd-net-device-module.h"
//#include "ns3/gtk-config-store.h"

using namespace ns3;


void
signalHandler (int signo)
{  
  if (signo == SIGINT)
    {
      Simulator::Stop();
      std::cout.flush ();
      std::cerr.flush ();
      exit (0);
    }
}


std::string
ipv6AddressToString (Ipv6Address addr)
{
  std::stringstream ss;
  std::string addrStr;
  addr.Print (ss);
  ss >> addrStr;
  return addrStr;
}

Ipv6Address
ipv6AddressFromArg(std::string arg)
{
  std::string ipv6Addr = arg.substr(0,arg.find("/"));
  return Ipv6Address (ipv6Addr.c_str());
}

Ipv6Prefix
ipv6PrefixFromArg(std::string arg)
{
  int ipv6Prefix = stoi(arg.substr(arg.find("/")+1));
  return Ipv6Prefix (ipv6Prefix);
}

Ipv6Address
ipv6NetworkFromArg(std::string arg)
{
  Ipv6Address ipv6Address = ipv6AddressFromArg(arg);
  Ipv6Prefix ipv6Prefix = ipv6PrefixFromArg(arg);
  return ipv6Address.CombinePrefix(ipv6Prefix);
}
/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */



NS_LOG_COMPONENT_DEFINE ("LteMutiUser");

int
main (int argc, char *argv[])
{

  uint16_t numberOfUeNodes = 1;
  uint16_t numberOfEnbNodes = 1;
  uint32_t noCooja = 1;
  double simTime = 10000;
  double distance = 0.5;
  double interPacketInterval = 1000;
  int seed = 1;
  bool useCa = false;
  std::string remoteHostAddrArg ("f3a1::1/64");
  std::string coojaAddrArg("fd00::201:1:1:1/64");
  std::string configFile("ns3CoojaConfig");
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  Config::SetDefault("ns3::RealtimeSimulatorImpl::SynchronizationMode", StringValue ("HardLimit"));
  Config::SetDefault("ns3::RealtimeSimulatorImpl::HardLimit", TimeValue(Time("5000000000ms")));

  if (signal (SIGINT,signalHandler) == SIG_ERR)
    {
      std::cerr << "Signal register error\n";
    }

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("remoteHostAddr","Remote host address with prefix", remoteHostAddrArg);
  cmd.AddValue("coojaAddr","Cooja GW address with prefix", coojaAddrArg);
  cmd.AddValue("numberOfUEs", "Number of UE nodes", numberOfUeNodes);
  cmd.AddValue("numberOfEnbs", "Number of eNodeB nodes", numberOfEnbNodes);
  cmd.AddValue("noCooja", "Number of cooja connections", noCooja);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.AddValue("seed", "Random seed", seed);
  cmd.Parse(argc, argv);

  RngSeedManager::SetSeed (seed);

  if (useCa)
   {
     Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
     Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
     Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
   }

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);




  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);


  Ipv6AddressHelper ipv6;
  ipv6.SetBase(ipv6NetworkFromArg(remoteHostAddrArg), ipv6PrefixFromArg(remoteHostAddrArg));
  Ipv6InterfaceContainer ifs6 = ipv6.Assign (internetDevices);
  Ipv6InterfaceAddress ipv6If = remoteHost->GetObject<Ipv6> ()->GetAddress (1,1);
  ipv6If.SetAddress (ipv6AddressFromArg (remoteHostAddrArg));
  remoteHost->GetObject<Ipv6> ()->RemoveAddress(1,1);
  remoteHost->GetObject<Ipv6> ()->AddAddress(1,ipv6If);
  Ipv6Address remoteHostAddr = ifs6.GetAddress (1,1);
  ifs6.SetForwarding(0,true);
  ifs6.SetForwarding(1,true);
  ifs6.SetDefaultRouteInAllNodes(1);

  Ipv6StaticRoutingHelper ipv6RoutingHelper;
  Ptr<Ipv6StaticRouting> remoteHostStaticRouting = ipv6RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv6> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv6Address ("7777:f00d::"), Ipv6Prefix (64), 1);
  remoteHostStaticRouting->AddNetworkRouteTo (ipv6NetworkFromArg(coojaAddrArg), ipv6PrefixFromArg(coojaAddrArg), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfEnbNodes);
  ueNodes.Create(numberOfUeNodes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAllocUe = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> positionAllocEnb = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfUeNodes; i++)
    {
      positionAllocUe->Add (Vector((numberOfEnbNodes/numberOfUeNodes) * distance * (i/2)*pow(-1,i), 0, 0));
    }
  for (uint16_t i = 0; i < numberOfEnbNodes; i++)
    {
      positionAllocEnb->Add (Vector(distance * (i/2)*pow(-1,i), 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAllocEnb);
  mobility.Install(enbNodes);
  mobility.SetPositionAllocator(positionAllocUe);
  mobility.Install(ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv6InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv6Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv6StaticRouting> ueStaticRouting = ipv6RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv6> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress6 (), 1);
    }


  NodeContainer coojaNodes (ueNodes.Get (0), remoteHost);
  CoojaFdNetDevicesHelper coojaHelper(configFile.c_str());
  if (noCooja > 0) coojaHelper.CreateGatewayNodes (coojaNodes);



  Ptr<Node> ueGW = ueNodes.Get(0);

  Ptr<Ipv6> ipv6Obj = ueGW->GetObject<Ipv6> ();
  uint32_t ueLteInterface = ipv6Obj->GetInterfaceForPrefix (Ipv6Address ("7777:f00d::"), Ipv6Prefix (64));
  ipv6Obj->SetForwarding (ueLteInterface, true);
  Ipv6Address ueGwAddr = ipv6Obj->GetAddress( ueLteInterface, 1).GetAddress();


  ipv6Obj = pgw->GetObject<Ipv6> ();
  uint32_t ueInterface = ipv6Obj->GetInterfaceForPrefix (Ipv6Address ("7777:f00d::"), Ipv6Prefix(64));
  Ptr<Ipv6StaticRouting> gwStaticRouting = ipv6RoutingHelper.GetStaticRouting(pgw->GetObject<Ipv6> ());
  gwStaticRouting->AddNetworkRouteTo (ipv6NetworkFromArg(coojaAddrArg), ipv6PrefixFromArg(coojaAddrArg), ueGwAddr,ueInterface);


  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numberOfUeNodes; i++)
      {
        lteHelper->Attach (ueLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated
      }


  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  for (uint32_t u = noCooja; u < ueNodes.GetN (); ++u) {
      ++ulPort;
      ++otherPort;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), ulPort));
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), otherPort));
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(100));

   }
  serverApps.Start (Seconds (200));

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  return 0;

}

