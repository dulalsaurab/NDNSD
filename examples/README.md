## NDNSD Examples

The examples are not build by default. Use option --with-examples with configure to build them i.e. ndnsd-consumer and ndnsd-consumer. 

`./waf configure --with-examples`  
`./waf build`

The example binaries (ndnsd-producer and ndnsd-consumer) can be found in build/examples. 
* Service Publisher: `ndnsd-producer`
* Service Finder: `ndnsd-consumer`

run `sudo ./waf install` to install the libraries + examples into the system (optional, but will be easy to use)

Once everything is installed, go through the following steps to run the consumer and the producer

#### Start NFD 

* start nfd -> `nfd-start &` '&' to run in backgroud.


#### start ndnsd-producer  
* `export NDN_LOG=ndnsd.*=TRACE`
* `ndnsd-producer service_name.info` (make sure `service_name.info` file is in the directory from where this command is executed)
* Sample output

```
1620755184.873257  INFO: [ndnsd.examples.ProducerApp] Starting producer application
1620755184.876101  INFO: [ndnsd.FileProcessor] Reading file: service_name.info
1620755184.876179 DEBUG: [ndnsd.FileProcessor] Key: descriptionValue: repo to insert sensor data
1620755184.876187 DEBUG: [ndnsd.FileProcessor] Key: appPrefixValue: /repo/r1
1620755184.876192 DEBUG: [ndnsd.FileProcessor] Key: servedPrefixValue: /prefix
1620755184.876195  INFO: [ndnsd.FileProcessor] Successfully updated the file content:
1620755184.876209 DEBUG: [ndnsd.SyncProtocolAdapter] Using PSync
1620755184.878455  INFO: [ndnsd.ServiceDiscovery] Setting/Updating producers state:
1620755184.878469  INFO: [ndnsd.ServiceDiscovery] Setting interest filter on: /repo/r1
1620755184.878592  INFO: [ndnsd.ServiceDiscovery] Advertising service under Name: /repo/r1
1620755184.878602 TRACE: [ndnsd.SyncProtocolAdapter] Publish sync update for prefix: /repo/r1
1620755184.878607  INFO: [ndnsd.SyncProtocolAdapter] Publishing update for: /repo/r1/1
1620755184.878636  INFO: [ndnsd.ServiceDiscovery] Publish: /repo/r1
1620755184.881154 DEBUG: [ndnsd.ServiceDiscovery] Successfully registered prefix: /repo/r1 
```

#### start ndnsd-consumer
* `export NDN_LOG=ndnsd.*=TRACE`
* `ndnsd-consumer -s repo` (for help, can run `ndnsd-consumer -h`)
* Sample output

```
1620755349.635997  INFO: [ndnsd.examples.ConsumerApp] Fetching service info for: repo
1620755349.637652 DEBUG: [ndnsd.SyncProtocolAdapter] Using PSync
1620755349.639744  INFO: [ndnsd.ServiceDiscovery] Requesting service: /repo
1620755349.640640  INFO: [ndnsd.SyncProtocolAdapter] Received PSync update event
1620755349.640651 DEBUG: [ndnsd.SyncProtocolAdapter] Sync update received for prefix: /repo/r1/1
1620755349.640660  INFO: [ndnsd.ServiceDiscovery] Fetching data for prefix:/repo/r1/%01
1620755349.640668 DEBUG: [ndnsd.ServiceDiscovery] Transmission count: 1 - Sending interest: /repo/r1/%01
1620755349.640673  INFO: [ndnsd.ServiceDiscovery] Sending interest: /repo/r1/%01
1620755349.641538  INFO: [ndnsd.ServiceDiscovery] Data received for: /repo/r1/%01
1620755349.641570  INFO: [ndnsd.examples.ConsumerApp] Service info received
1620755349.641573  INFO: [ndnsd.examples.ConsumerApp] Status: EXPIRED
1620755349.641575  INFO: [ndnsd.examples.ConsumerApp] Callback: appPrefix:/repo/r1
1620755349.641577  INFO: [ndnsd.examples.ConsumerApp] Callback: description:repo to insert sensor data
1620755349.641579  INFO: [ndnsd.examples.ConsumerApp] Callback: servedPrefix:/prefix
1620755349.641581  INFO: [ndnsd.examples.ConsumerApp] Callback: service-name:/repo/r1
```



