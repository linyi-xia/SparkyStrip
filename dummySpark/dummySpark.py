#!/usr/bin/env python3
'''
Created on Apr 7, 2016

@author: Jeffrey Thompson
'''


import MySQLdb, time


input_file = open("dummy_data.csv")
db = MySQLdb.connect("localhost","u123","sparkystrip_device","SparkyStrip" )

input("Connected!\nPress Enter to start the first dataset")
for line in input_file:
	line = line.rstrip()
	if line == '':
		input("Dataset finished, press enter to continue")
	else:
		time.sleep(1)
		cursor = db.cursor()
		cursor.execute( 'CALL SparkyStrip.PushData({});'.format(line) )
		db.commit()
		print('Pushing:',line)
    
print('Pushed all data, exiting\n');
db.close()
input_file.close()
