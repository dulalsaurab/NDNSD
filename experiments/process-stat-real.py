
import time
import subprocess
from collections import defaultdict
from matplotlib import pyplot as plt
import csv 
import numpy as np

rootDir = "/tmp/minindn"
# rootDir = "real-exp/after_timesync/congestion/producer++/3c4p-500ms"

topo = {
        'c1p1':1, 'c1p2':1, 'c1p3':1, 'c1p4':1, 'c1p5':1,
        'c2p1':1, 'c2p2':1, 'c2p3':1, 'c2p4':1, 'c2p5':1,
        'c3p1':1, 'c3p2':1, 'c3p3':1, 'c3p4':1, 'c3p5':1,
        # 'c4p1':1, 'c4p2':1, 'c4p3':1,
        # 'c5p1':1, 'c5p2':1, 'c5p3':1,
        # 'c6p1':1, 'c6p2':1, 'c6p3':1,
        # 'c8p1':1, 'c8p2':1,
        }
prod = ['p1', 'p2', 'p3', 'p4', 'p5']
cons = ['c1','c2', 'c3']
# topo = {'c1p1' : 1}
# prod = ['p1']
# cons = ['c1']


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
# <<<<<<< HEAD
    
#     send = processLogFile(file1, ["Transmission count: 1 - Sending interest: /ndnsd/p1",
#                                   "Transmission count: 1 - Sending interest: /ndnsd/p2",
#                                   "Transmission count: 1 - Sending interest: /ndnsd/p3",
#                                   "Transmission count: 1 - Sending interest: /ndnsd/p4",
#                                   "Transmission count: 1 - Sending interest: /ndnsd/p5"
#                                   ])

    
#     received = processLogFile(file2, ["Data received for: /ndnsd/p1/",
#                                       "Data received for: /ndnsd/p2/",
#                                       "Data received for: /ndnsd/p3/",
#                                       "Data received for: /ndnsd/p4/",
#                                       "Data received for: /ndnsd/p5/"
#                                       ])
# =======
    grep_string_send = ["Transmission count: 1 - Sending interest: /ndnsd/{}".format(x) for x in prod]
    grep_string_receive = ["Data received for: /ndnsd/{}".format(x) for x in prod]

    send = processLogFile(file1, grep_string_send)

    
    received = processLogFile(file2, grep_string_receive)
# >>>>>>> 3a759e457ca3dc604c94d8c99a57f950a61d9118

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
    # print(sync_d)
    # exit()
    dump(sync_d[idx], str(idx)+'.csv')
  
  # for i in hop:
  #   print(hop[i])
    # dump(hop[i], str(i)+'.csv')


