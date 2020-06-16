
import time
import subprocess
from collections import defaultdict
from matplotlib import pyplot as plt
import csv 

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
  with open(filename, 'wb') as f:
    writer = csv.writer(f)
    for i in range(len(data[0])):
      writer.writerow([x[i] for x in data])

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
      r_final[key] = None
  return r_final

if __name__ == '__main__':

  rootDir = "/tmp/minindn"
  topo = {'ap1':1, 'bp2':1,'ap2':2, 'bp1':2}

  cons = ['a', 'b']
  prod = ['p1', 'p2']
  
  # s_g = [] #send grep
  # r_g =[] #receiver grep
  # for i in prod:
  #     s_g.append("Sending interest: /{}".format(i))
  #     r_g.append("Data received for: /{}".format(i))
  hop = defaultdict(list)
  for i in cons:
    file1 = "{}/{}/consumer.log".format(rootDir, i)
    file2 = "{}/{}/consumer.log".format(rootDir, i)
    
    send = processLogFile(file1, ["Sending interest: /p1", "Sending interest: /p2"])
    received = processLogFile(file2, ["Data received for: /p1", "Data received for: /p2"])
    r_final = getDiff(send, received)
    for p in prod:
      hop[topo[i+p]].append([r_final[x] for x in r_final if p in x])

  for i in hop:
    dump(hop[i], str(i)+'.csv')





