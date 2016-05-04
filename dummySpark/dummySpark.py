#!/usr/bin/env python3
'''
Created on Apr 7, 2016

@author: Jeffrey Thompson
'''


import MySQLdb, time, sys
# sudo apt-get install python-dev libmysqlclient-dev
# sudo apt-get install python3-pip
# pip3 install mysqlclient

if len(sys.argv) < 2:
	print('please pass the filename(s) with the data')
	sys.exit()

try:
	db = MySQLdb.connect("72.219.144.187","u123","sparkystrip_device","SparkyStrip" )
	cursor = db.cursor()
except Exception:
	print('Could not connect to the server!\n')
	sys.exit()
	
for filename in sys.argv[1:]:
	try:
		parts = filename.split('.')
		if len(parts) > 1:
			if parts[-1] != 'csv':
				continue
		else:
			filename += '.csv'
		input_file = open(filename)
		print("Pushing data contained in file",filename, end='')
		for line in input_file:
			line = line.rstrip()
			try:
				cursor.execute( 'CALL PushData({});'.format(line) )
				db.commit()
				print('.', end='',flush=True)
			except Exception as e:
				print('Failed to push, ERROR:',e)
			time.sleep(1)
		input_file.close()
		print()
	except OSError as e:
		print("Could not open file", filename, ":",e)

    
print('Finished!\n');
db.close()

