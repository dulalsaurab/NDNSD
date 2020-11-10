
import time
import subprocess
from collections import defaultdict
# from matplotlib import pyplot as plt
import csv 
import numpy as np

rootDir = "/home/vagrant/mini-ndn/ndn-src/ndnsd/experiments/result/loss-experiment/10_percent"
# rootDir = "real-exp/after_timesync/congestion/producer++/3c4p-500ms"

topo = { 
        'c1p1':3, 'c1p2':4,
        'c2p1':2, 'c2p2':3,
        'c3p1':1, 
        'c4p1':3, 'c4p2':2,
        'c5p1':4, 'c5p2':3
        }
prod = ['p1', 'p2']
cons = ['c1','c2', 'c3', 'c4', 'c5']

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
    prefixPublishTS = processLogFile(file1, ["Publishing update for: /ndnsd/{}/".format(p)])

    for c in cons:
      file2 = "{}/{}/consumer.log".format(rootDir, c)
      prefixUpdateTS = processLogFile(file2,  ["Sync update received for prefix: /ndnsd/{}/".format(p)])

      r_final = getDiff(prefixPublishTS, prefixUpdateTS)
      c_p_sync = [r_final[x] for x in r_final]
      header = "{} - {} sync".format(p, c)
      c_p_sync.insert(0, header)
      try:
        hop[topo[c+p]].append(c_p_sync)
      except Exception as e:
        print(e)
        # reach here if the link doesn't exist
  return hop

def computeServiceInfoFetchDelay():
  hop = defaultdict(list)
  for i in cons:
    conLog = "{}/{}/consumer.log".format(rootDir, i)
    # file2 = "{}/{}/consumer.log".format(rootDir, i)

    grep_string_send = ["Transmission count: 1 - Sending interest: /ndnsd/{}".format(x) for x in prod]
    grep_string_receive = ["Data received for: /ndnsd/{}".format(x) for x in prod]

    send = processLogFile(conLog, grep_string_send)
    received = processLogFile(conLog, grep_string_receive)
    r_final = getDiff(send, received)

    for p in prod:
      c_p = [r_final[x] for x in r_final if p in x]
      header = "{} - {}".format(i, p)
      c_p.insert(0, header)
      try:
        hop[topo[i+p]].append(c_p)
      except Exception as e:
        print (e)
        pass
  return hop

if __name__ == '__main__':

  sync_d = computeSyncDelay()
  info_d = computeServiceInfoFetchDelay()

  for idx in sync_d:
    sync_d[idx] = sync_d[idx] + info_d[idx]
    print(sync_d)
    dump(sync_d[idx], str(idx)+'.csv')