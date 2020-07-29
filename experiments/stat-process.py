
import time
import subprocess
from collections import defaultdict
from matplotlib import pyplot as plt
import csv 
import numpy as np

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

def dump(data, filename):
  h_l = [item[0] for item in data] 
  data = np.transpose([i[1:] for i in data])
  header = ','.join(h_l)
  np.savetxt(filename, data, delimiter=",", header=header, fmt='%1.5f')

def drawGraph(r_final):
  pass

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

if __name__ == '__main__':

  rootDir = "/Users/saurabdulal/Documents/IMAGES/mini-ndn/ndn-src/NDNSD/experiments/real-exp/phase2/congestion/3p6c-100ms/take1"
  topo = {'c1p1':1, 'c1p2':1, 'c1p3':1,
          'c2p1':1, 'c2p2':1, 'c2p3':1,
          'c3p1':1, 'c3p2':1, 'c3p3':1,
          'c4p1':1, 'c4p2':1, 'c4p3':1,
          'c5p1':1, 'c5p2':1, 'c5p3':1,
          'c6p1':1, 'c6p2':1, 'c6p3':1,
          }

  prod = ['p1', 'p2', 'p3']
  cons = ['c1', 'c2', 'c3', 'c4', 'c5', 'c6']

  # topo = {'c1p1':1}
  # prod = ['p1']
  # cons = ['c1']

  hop = defaultdict(list)
  for i in cons:
    file1 = "{}/{}/consumer.log".format(rootDir, i)
    file2 = "{}/{}/consumer.log".format(rootDir, i)
    
    send = processLogFile(file1, [
                                  "Transmission count: 1 - Sending interest: /ndnsd/p1",
                                  # "Transmission count: 1 - Sending interest: /p2/",
                                  # "Transmission count: 1 - Sending interest: /p3/",
                                  ])
    received = processLogFile(file2, [
                                      "Data received for: /ndnsd/p1/", 
                                      # "Data received for: /p2", 
                                      # "Data received for: /p3"
                                      ])

    r_final = getDiff(send, received)
    for p in prod:
      c_p = [r_final[x] for x in r_final if p in x]
      header = "{} - {}".format(i, p)
      c_p.insert(0, header)
      hop[topo[i+p]].append(c_p)

  for i in hop:
    dump(hop[i], str(i)+'.csv')


