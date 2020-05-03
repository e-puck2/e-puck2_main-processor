import serial
import time
import struct
import cv2
import numpy as np

width = 160
height = 120

ser = serial.Serial('COM181', 2000000, timeout=0.1)

# flush input and output buffers
ser.reset_input_buffer()
ser.reset_output_buffer()

message = struct.pack(">bb", - ord('I'), 0)
ser.write(message)
ser.flush()
print("cmd = " + str(message))

reply = ser.read()
while len(reply) < width*height+3:
	print("read " + str(len(reply)) + " bytes")
	reply += ser.read()
	
print("mode=" + str(reply[0]) + ", w=" + str(reply[1]) + ", h=" + str(reply[2]))
print("image received")
	
ser.close()

out = open("image.bayer", "wb")
out.write(reply[3:])
out.close()

path = 'image.bayer'
f = open(path,'rb')
bin_image = np.fromstring(f.read(), dtype=np.uint8)
bin_image.shape = (height, width)

cv2.imwrite("bayer.jpg", bin_image, [int(cv2.IMWRITE_JPEG_QUALITY), 70])

temp_img = cv2.cvtColor(bin_image, cv2.COLOR_BayerGB2BGR)
cv2.imwrite("COLOR_BayerGB2BGR.jpg", temp_img, [int(cv2.IMWRITE_JPEG_QUALITY), 70])

#print("image size = " + str(bin_image.shape))
#cv2.imshow('image', temp_img) 




