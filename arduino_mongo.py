#!/usr/bin/env python3
__author__ = 'Jeffrey Thompson' #adapted from Edmunds script

import sys
import socket
import json
import datetime
from pymongo import MongoClient
import struct
import subprocess
import time

STUCT_FORMAT= 'Q'+'f'*9
MONGO_URI = 'mongodb://sparkystrip:calplug123@ds011429.mlab.com:11429/sparkystrip'
MONGO_URI = 'mongodb://Dpynes:Pizzahead9@ds011860.mlab.com:11860/projectdp'
MONGO_DATABASE = 'projectdp'
MONGO_COLLECTION = 'rawData'
HOST = ''
PORT = 12021


# funky function but works - edmunds work
def return_int_ip() -> int :
    self_ip = [(s.connect(('8.8.8.8', 80)), s.getsockname()[0], s.close()) for s in [socket.socket(socket.AF_INET, socket.SOCK_DGRAM)]][0][1]
    return struct.unpack("!I", socket.inet_aton(self_ip))[0]


######## start of script ########




mongo_connect = MongoClient(MONGO_URI)
mongo_db = mongo_connect[MONGO_DATABASE]
mongo_collection = mongo_db[MONGO_COLLECTION]

while True:
    try:
        dirty_sock= socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        dirty_sock.bind((HOST, PORT))
        break
    except OSError:
        print( 'killing old process hogging the port' )
        subprocess.call( 'lsof -i :{} | grep -i "python" | cut -d " " -f3 | xargs kill -9'.format(PORT), shell=True )
        time.sleep(1)


print("IP FOR THIS DEVICE : ", return_int_ip())
print("Port :",PORT);


out_file = open('Local_Data.csv', 'a')
out_file.write('timestamp, deviceID, Power, Power_Factor ,Real60, Real180, Real300, Real420, Imm60, Imm180, Imm 300, Imm 420\n')

while True:
    try :
        
        raw_data = dirty_sock.recv(1024)
        
        timestamp = str(datetime.datetime.utcnow())
        
        usable_data = struct.unpack(STUCT_FORMAT, raw_data)
        if usable_data[0] == 0:
            break
        
        json_dict = json.loads( [{'timestamp': timestamp,
                                 'deviceID': usable_data[0],
                                 'data' : usable_data[1:],
                                 }] )
        
        pretty = timestamp + ',' + ','.join(str(x) for x in usable_data + '\n'
        out_file.write(pretty)
        out_file.flush()
        
        print('Inserting data : ', pretty)
        mongo_collection.insert(json_dict)

    except Exception as error:
        print('ERROR : ', str(error))
        continue

