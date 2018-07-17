
var folder=".";
var packets = new Array();
var flushCycle = 10000;
var metrics = ['pdr','delay','jitter','throughput'];
var rpl_stats = {}

var allPacketsFile = new java.io.FileWriter(folder + "/log_allp.log");
var energyFile = new java.io.FileWriter(folder + "/log_energy.log");
var fluxFile =  new java.io.FileWriter(folder + "/flux.log",true);
var fluxHeaderFile = new java.io.FileWriter(folder + "/flux-header.log");
var finalFile = new java.io.FileWriter(folder + "/final.log",true);


function flatPacketList(){
  var all_packets = [];
  for(var flux in packets){
    for(var stream in packets[flux]){
      for(var id in packets[flux][stream]){
        var p = packets[flux][stream][id];
        p.flux = flux;
        p.stream = stream;
        p.id = id;
        all_packets.push(p);
      }
    }
  }
  return all_packets;
}

function endCalculations(){
  var str="";

  /*initialize stats*/
  var stats = {};

  var all_packets = flatPacketList();
  for(var i = 0; i < all_packets.length; i++){
    var p = all_packets[i];
    stats[p.stream] = stats[p.stream] || {};
    if(!stats[p.stream][p.flux]){
      stats[p.stream][p.flux] = {};
      stats[p.stream][p.flux].sent = 0;
      stats[p.stream][p.flux].recv = 0;
      for(var m = 0; m < metrics.length; m++){
        stats[p.stream][p.flux][metrics[m]]=0;
      }
    }

    stats[p.stream][p.flux].sent += 1;
    log.log("s: " + p.sent + ", ");
    if(p.recv){
      stats[p.stream][p.flux].recv +=1;
      stats[p.stream][p.flux].delay += (p.recv - p.sent)/1000;
      log.log("r: " + p.recv + ", ");
    }
  }
  log.log("\n");

  for(var stream in stats){
    for(var flux in stats[stream]){
      stats[stream][flux].delay /= stats[stream][flux].recv;
      stats[stream][flux].pdr = stats[stream][flux].recv / stats[stream][flux].sent;
    }
  }

  for(var i = 0; i < all_packets.length; i++){
    var p = all_packets[i];
    if(p.recv){
      stat = stats[p.stream][p.flux];
      stats[p.stream][p.flux].jitter += Math.abs(stats[p.stream][p.flux].delay - (p.recv - p.sent)/1000)/ stats[stream][flux].recv;
      if(p.size){
        stats[p.stream][p.flux].throughput += (p.size/stats[p.stream][p.flux].delay)/stats[p.stream][p.flux].recv;
      }
    }
  }
  log.log(JSON.stringify(stats) + "\n");

  //TODO log stats by flux

  for(var stream in stats){
    for(var i = 0; i < metrics.length; i++){
      var metric = metrics[i];
      var cnt = 0;
      stats[stream][metric] = stats[stream][metric] || {};
      stats[stream][metric].average = 0;
      stats[stream][metric].stddev = 0;
      for(var flux in stats[stream]){
        if(!stats[stream][flux][metric]) continue;
        cnt++;
        stats[stream][metric].average += stats[stream][flux][metric];
      }
      stats[stream][metric].average /= cnt;
      for(var flux in stats[stream]){
        if(!stats[stream][flux][metric] || metrics.indexOf(flux) != -1) continue;

        stats[stream][metric].stddev += Math.pow(stats[stream][metric].average - stats[stream][flux][metric],2);
      }
      stats[stream][metric].stddev /= cnt;
      stats[stream][metric].stddev = Math.sqrt(stats[stream][metric].stddev);
      str+= stream + "_" + metric + "\t" + stats[stream][metric].average + "\t" + stats[stream][metric].stddev +"\n";
    }
  }
  var flux_str = "";
  var used_metrics = [];
  for(var stream in stats){
    for(var flux in stats[stream]){
      if(metrics.indexOf(flux) != -1) continue;
      flux_str += stream + "\t" + flux;
      for(var i = 0; i < metrics.length; i++){
        var metric = metrics[i];
        if(!stats[stream][flux][metric]) continue;
        if(used_metrics.indexOf(metric) == -1) used_metrics.push(metric);
        flux_str += "\t" + stats[stream][flux][metric];
      }
      flux_str += "\n";
    }
  }
  var flux_hdr = "Stream\tFlux";
  for(var i = 0; i < used_metrics.length; i++){
    flux_hdr += "\t" + used_metrics[i];
  }
  flux_hdr += "\n";
  fluxHeaderFile.write(flux_hdr);
  fluxHeaderFile.close();
  fluxFile.write(flux_str);
  fluxFile.close();

  finalFile.write(str);
  for(var key in rpl_stats){
    var avg = 0;
    var cnt = 0;
    for(var node in rpl_stats[key]){
      avg += rpl_stats[key][node];
      cnt += 1;
    }
    finalFile.write(key + '\t' + avg/cnt + '\n');
  }
  finalFile.close();
};

function flushFiles(){
  energyFile.flush();
  allPacketsFile.flush();
  finalFile.flush();
}

function closeFiles(){
  energyFile.close();
  allPacketsFile.close();
  finalFile.close();
}

TIMEOUT(3660000);

var iteration = 0;

while(1) {
  try{
    YIELD();
  }catch(e){
    endCalculations();
    closeFiles();
    log.testOK();
  }
  if(sim.getSimulationTimeMillis() > 3660000 - 60000){
    endCalculations();
    closeFiles();
    log.testOK();
  }

  log.log(msg + "\n");

  iteration+=1;
  if(iteration%flushCycle == 0) flushFiles();

  if(msg.startsWith("{")) {
    var pinfo = eval("(" + msg + ")");
    pinfo.node = id;
    pinfo.time = sim.getSimulationTime();
    if(pinfo.type == 'Stats'){
      rpl_stats[pinfo.key] = rpl_stats[pinfo.key] || {};
      rpl_stats[pinfo.key][pinfo.node] = pinfo.value;
    } else if(pinfo.action == 'Send'){
      var stream = pinfo.type;
      var flux = pinfo.from + "-" + pinfo.to;
      var packet_id = pinfo.object.id;
      packets[flux] = packets[flux] || {};
      packets[flux][stream] = packets[flux][stream] || {};
      packets[flux][stream][packet_id] = {sent: sim.getSimulationTime()};
    }else if(pinfo.action == 'Recv'){
      var stream = pinfo.type;
      var flux = pinfo.from + "-" + pinfo.to;
      var packet_id = pinfo.object.message.id;
      pinfo.object.id = pinfo.object.message.id;
      if(!packets[flux][stream][packet_id].recv)
        packets[flux][stream][packet_id].recv = sim.getSimulationTime();
    }else if(pinfo.action == 'Acting'){
      var stream = pinfo.type;
      var flux = pinfo.from + "-" + pinfo.to;
      var packet_id = pinfo.object.message.id;
      pinfo.object.id = pinfo.object.message.id;
      if(!packets[flux][stream][packet_id].recv)
        packets[flux][stream][packet_id].recv = sim.getSimulationTime();
    }
    allPacketsFile.write(JSON.stringify(pinfo) + "\n");
  }else if(msg.startsWith("#P")){
    var msgArray = msg.split(' ');
    energyFile.write(
      id + " " + msgArray[1] + " " + msgArray[3] +  " " + msgArray[4] + " " + msgArray[5] + " " + msgArray[6] +"\n");
  }
}
