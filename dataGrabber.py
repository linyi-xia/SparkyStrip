#!/usr/bin/env python3
'''
Created on Apr 7, 2016

@author: Jeffrey Thompson
'''


import serial, time, struct, sys,os

CYCEND = 1

if len(sys.argv) < 2:
	print('please pass the device name')
	sys.exit()
	
file_num = ''
file_name_template = 'data/'+sys.argv[1]+'{}.csv'
dump = False

with serial.Serial('/dev/tty.usbmodem1421', 115200) as ser:
	time.sleep(2)
	ser.flush()
	while True:
		filename = file_name_template.format(file_num)
		while os.path.exists(filename):
			if file_num:
				file_num+=1
			else:
				file_num = 1
			filename = file_name_template.format(file_num)
			
		while True:
			try:  #the decode sometimes fails
				line = ser.readline().decode("utf-8")
				if line == "Begin SparkyStrip!\n":
					break
				if line == "Begin DataDumper!\n":
					dump = True 
					break
			except (...):
				pass
			
		with open(filename, 'w') as out_file:
			if dump:
				print("syncronized, Dumping data to ", filename, end='',flush=True)
				out_file.write( ser.readline().decode("utf-8"))
				while True:
					wave_raw = ser.read(3)
					# 1 bit ZX + 23 bits waveform (no data lost as it really uses 22 bits)
					val = struct.unpack('<l', wave_raw+b'\x00')[0];
					zx = val>>23
					wave = val & 0x7fffff
					if wave & (1<<22):
						print(wave)
						# bit 22 is set, we assume negative value
						# so sign extend and interpret as a signed 32bit number
						wave = struct.unpack('<l', struct.pack('<L',wave | 0xff800000) )[0]
						print('  ',wave)
					out_file.write( '{},{}\n'.format(zx, wave) )
			else:
				print("syncronized, Saving data to ", filename, end='',flush=True)
				while True:
					out_file.write( ser.readline().decode("utf-8"))
					out_file.flush()
					print(end='.',flush=True)
