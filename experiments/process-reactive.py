import os
import subprocess
from collections import defaultdict
import csv 

result_folder_c = ['wu', 'arizona', 'uiuc', 'uaslp', 'csu']
result_folder_p = ['memphis', 'ucla']

def processResults(rootdir):
  # process producer
  result_dict = defaultdict(dict)
  for p in result_folder_p:
      file = rootdir+"/" + p + "/producer.log"
      with open (file, 'r', encoding='utf-8',
                 errors='ignore') as f:
        for line in f:
          if line and "Service updated " in line:
            a = line.split()
            prefix = "/uofm/{}/{}".format(p, int(a[8])+1)
            timestamp = a[0]
            # if (p == 'p1'):
              # print (prefix, timestamp)
            result_dict[p][prefix] = timestamp
  
  final_result = []
  for c in result_folder_c:
    temp1 = [] 
    temp2 = []
    temp1.append("memphis -"+c)
    temp2.append("ucla -"+c)

    file = rootdir + "/" + c + "/consumer.log"
    with open (file, 'r', encoding='utf-8', errors='ignore') as f:
      for line in f:
        if line and "Data content:" in line:
          a = line.split()
          temp = a[5].split("|")
          prefix = "{}/{}".format(temp[0], temp[1])
          timestamp = a[0]
          if prefix in result_dict[c]:  
            continue

          result_dict[c][prefix] = a[0]
          try:
            if 'memphis' in prefix:
              delay = float(timestamp) - float(result_dict['memphis'][prefix])
              temp1.append(delay)
            if 'ucla' in prefix:
              delay = float(timestamp) - float(result_dict['ucla'][prefix])
              temp2.append(delay)
          except Exception as e:
            print (e)

    final_result.append(temp1)
    final_result.append(temp2)

  with open ("2-loss-reactive.csv", 'w') as f:
    write = csv.writer(f)
    write.writerows(final_result)

if __name__ == '__main__':
    processResults("/home/mini-ndn/europa_bkp/mini-ndn/sdulal/mini-ndn/ndn-src/NDNSD/experiments/result/comparision/p_interval_10/reactive/loss_2")
