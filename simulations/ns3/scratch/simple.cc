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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/fd-net-device-module.h"

using namespace ns3;



NS_LOG_COMPONENT_DEFINE ("Ns3CoojaSimple");

int
main (int argc, char *argv[])
{

  double simTime = 10000;
  std::string configFile("ns3CoojaDirect");
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  Config::SetDefault("ns3::RealtimeSimulatorImpl::SynchronizationMode", StringValue ("HardLimit"));
  Config::SetDefault("ns3::RealtimeSimulatorImpl::HardLimit", TimeValue(Time("10ms")));

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();


  NodeContainer nodeC;
  nodeC.Create(1);
  
  InternetStackHelper internet;
  internet.Install(nodeC);

  NodeContainer coojaNodes (nodeC.Get (0), nodeC.Get (0));
  CoojaFdNetDevicesHelper coojaHelper(configFile.c_str());
  coojaHelper.CreateGatewayNodes (coojaNodes);

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  Simulator::Destroy();
  return 0;

}

