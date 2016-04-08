#!/usr/bin/env python3
'''
Created on Apr 7, 2016

@author: Jeffrey Thompson
'''


import MySQLdb, time
# apt-get install python-dev libmysqlclient-dev
# apt-get install pip3
# pip3 install mysqlclient

#def print(*args):
#	pass

db = MySQLdb.connect("localhost","sparkyID","squidsofpink","SparkyStrip" )
db.autocommit(True)
myUpdateNum = 0
apps = tuple()

print("Connected!\nWaiting for unprocessed data")
while True:
	cursor = db.cursor()
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
			print( 'DataID: {}, Distance: {}, Identity: {}'.format(dataID,dist,identity) )
			cursor.execute( "INSERT INTO DeviceHistory(dataID,appName) VALUES({},'{}');".format(dataID,identity) )
		else:
			print('Nothing trained!')
	else:
		time.sleep(1)

    
print('Pushed all data, exiting\n');
db.close()
input_file.close()
