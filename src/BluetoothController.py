import json
import httplib, urllib
import redis
import ipdb

MAC_TO_USER_HS = 'mac_to_user_HS'
REDIS_BLUETOOTH_CTR_CH = 'bluetooth_controller_CH'

mac_to_user_mapping = json.load(urllib.urlopen('http://www.stonesoup.im/bluetooth/sync/'))

r = redis.StrictRedis(host="gauravparuthi.com", port=6379, db=0)

rLocal = redis.StrictRedis(host="localhost", port=6379, db=0)

rLocal.hmset(MAC_TO_USER_HS, mac_to_user_mapping)

rPubSub = r.pubsub()
rPubSub.subscribe([REDIS_BLUETOOTH_CTR_CH]) 

for item in rPubSub.listen():
	print item 		
	if item['type'] == 'message':
		mapping = json.loads(item['data'])
		mac_addr = mapping.keys()[0]
	
		# mac_addr, username
		rLocal.hset(MAC_TO_USER_HS, mac_addr, mapping[mac_addr])
	
		#ipdb.set_trace()
	 		
