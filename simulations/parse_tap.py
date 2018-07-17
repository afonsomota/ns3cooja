#!/usr/bin/python3
import sys
import numpy


def get_waist(if1,if2):
  
  if1_file = open("log_if"+if1+".pcap.log")
  if2_file = open("log_if"+if2+".pcap.log")
  
  if1_packets = {}
  if2_packets = {}
  
  files = [if1_file, if2_file]
  packets = [if1_packets, if2_packets]
  
  for i in [0,1]:
    f = files[i]
    for l in f.readlines():
      line = l.split("\t")
      key = line[1] + "-" + line[2] + "-" + line[3]
      packets[i][key] = float(line[0])
  
  count = 0
  delivered = 0
  delay = []
  for k in if1_packets:
    count += 1
    if k in if2_packets:
      delivered +=1
      delay.append(if2_packets[k]-if1_packets[k])

  return {"count":count, "delivered": delivered, "delay": delay, "pdr": delivered*1.0/count}
  

delay = 0
pdr = 0
std = 0

if len(sys.argv) > 3:
  waist1 = get_waist(sys.argv[1],sys.argv[2])
  waist2 = get_waist(sys.argv[3],sys.argv[4])
  
  delay = numpy.mean(waist1["delay"]) + numpy.mean(waist2["delay"])
  std = numpy.std(waist1["delay"]) + numpy.std(waist2["delay"])
  
  pdr = waist1["pdr"]*waist2["pdr"]

else:
  waist = get_waist(sys.argv[1],sys.argv[2])
  delay = numpy.mean(waist['delay'])
  std   = numpy.std(waist['delay'])
  pdr   = waist['pdr']




print("WaistDelay",delay*1000,std*1000,sep="\t")  
print("WaistDelivery",pdr,0,sep="\t")
