
import time
import subprocess
from collections import defaultdict
from matplotlib import pyplot as plt
import csv 
import numpy as np

rootDir = "emulation/1p1c-1/take2"
rootDir = "real-exp/after_timesync/distance/1p1c-1/1500ms"
topo = {'c1p1':1}
prod = ['p1']
cons = ['c1']

def processLogFile(filename, searchStrings):
  res_dict = {}
  with open (filename, 'r') as f:
    _row = f.readlines()
    for row in _row:
      for sstring in searchStrings:
        if sstring in row:
          arr = row.split(" ")
          res_dict[arr[-1].strip()] = arr[0]
  return res_dict

def getDiff(send, received):
  r_final = {}
  for key in send:
    st = send[key]
    if key in received:
      rt = received[key]
      r_final[key] = 1000*(float(rt) - float(st))
    else:
      r_final[key] = 0
  return r_final

def dump(data, filename):
  h_l = [item[0] for item in data] 
  data = np.transpose([i[1:] for i in data])
  header = ','.join(h_l)
  np.savetxt(filename, data, delimiter=",", header=header, fmt='%1.5f')

def drawGraph(r_final):
  pass

def computeSyncDelay():
  hop = defaultdict(list)
  for p in prod:
    file1 = "{}/{}/producer.log".format(rootDir, p)
    prefixPublishTS = processLogFile(file1,
                                    ["Publishing update for: /ndnsd/{}/".format(p)])

    for c in cons:
      file2 = "{}/{}/consumer.log".format(rootDir, c)
      prefixUpdateTS = processLogFile(file2, 
                                      ["Sync update received for prefix: /ndnsd/{}/".format(p)])

      r_final = getDiff(prefixPublishTS, prefixUpdateTS)
      c_p_sync = [r_final[x] for x in r_final]
      header = "{} - {} sync".format(p, c)
      c_p_sync.insert(0, header)
      hop[topo[c+p]].append(c_p_sync)

  return hop


def computeServiceInfoFetchDelay():
  hop = defaultdict(list)
  for i in cons:
    file1 = "{}/{}/consumer.log".format(rootDir, i)
    file2 = "{}/{}/consumer.log".format(rootDir, i)
    
    send = processLogFile(file1, ["Transmission count: 1 - Sending interest: /ndnsd/p1"])
    received = processLogFile(file2, ["Data received for: /ndnsd/p1/"])

    r_final = getDiff(send, received)

    for p in prod:
      c_p = [r_final[x] for x in r_final if p in x]
      header = "{} - {}".format(i, p)
      c_p.insert(0, header)
      hop[topo[i+p]].append(c_p)

  return hop

if __name__ == '__main__':

  # hop = computeServiceInfoFetchDelay()
  sync_d = computeSyncDelay()
  info_d = computeServiceInfoFetchDelay()
  for idx in sync_d:
    sync_d[idx] = sync_d[idx] + info_d[idx]
    dump(sync_d[idx], str(idx)+'.csv')
  
  # for i in hop:
  #   print(hop[i])
    # dump(hop[i], str(i)+'.csv')


