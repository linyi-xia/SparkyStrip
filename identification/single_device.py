#!/usr/bin/env python3
'''
Created on Apr 7, 2016

@author: Jeffrey Thompson
'''


import MySQLdb, time
# sudo apt-get install python-dev libmysqlclient-dev
# sudo apt-get install python3-pip
# pip3 install mysqlclient

#def print(*args):
#	pass

'''
out_file = open('Local_Data.csv', 'a')
if out_file.tell() = 0:
	# if the file is empty write the header
	out_file.write('timestamp, dataID, Power, data...\n')
'''

myUpdateNum = 0
apps = tuple()

while True:
	try:
		db = MySQLdb.connect("localhost","sparkyID","squidsofpink","SparkyStrip" )
		db.autocommit(True)
		cursor = db.cursor()
		print("Connected!")
		while True:
			val = cursor.execute( 'CALL getUnprocessed();' )
			if val:
				print('\npulling data')
				updateNum, dataID, *data = cursor.fetchone()
				print('Recieved:', updateNum,dataID,data)
				if updateNum != myUpdateNum:
					print('updating appliance stats')
					cursor.execute( 'SELECT * FROM Appliances;' )
					apps = cursor.fetchall();
					myUpdateNum = updateNum
				identity = None
				min_dist = 9999999999999
				for app in apps:
					dist = 0
					for i,x in enumerate(data,1):
						dist += (x-app[i])**2
					dist **= .5
					print(app[0],':',dist)
					if dist < min_dist:
						min_dist = dist
						identity = app[0]
				if identity:
					print( 'DataID: {}, Distance: {}, Identity: {}'.format(dataID,min_dist,identity) )
					cursor.execute( "INSERT INTO DeviceHistory(dataID,appName) VALUES({},'{}');".format(dataID,identity) )
				else:
					print('Nothing trained!')
			else:
				time.sleep(1)
	except MySQLdb.Error:
		db.close()
		time.sleep(5)
		print("retrying to connect")
	

