import serial
import serial_datagram
import sys
import os
import msgpack

#this script needs Python 3, pyserial and msgpack to run
#pip3 install pyserial
#pip3 install msgpack-python

#the script need the serial port to open as arg when executing it
def main():
    if len(sys.argv) > 2:
        baud = sys.argv[2]
    else:
        baud = 115200

    if len(sys.argv) == 1:
        print('Please give the serial port to use')
        return
    else:
        fdesc = serial.Serial(sys.argv[1], baudrate=baud)

    while True:
        try:
            #return a complete decoded frame
            dtgrm = serial_datagram.read(fdesc)
            #unpack the message pack frame
            data = msgpack.unpackb(dtgrm, use_list=False, encoding='utf-8') 

            padding = len('accelerometer')

            #print acc datas only if they are present in the frame received
            field_to_found = 'acc'
            if(field_to_found in data):
                buf = data[field_to_found]
                print('{} : x = {:6.2f}, y = {:6.2f}, z = {:6.2f}'.format('accelerometer',buf[0], buf[1], buf[2]))

            #print gyro datas only if they are present in the frame received
            field_to_found = 'gyro'
            if(field_to_found in data):
                buf = data[field_to_found]
                print('{:<{}} : x = {:6.2f}, y = {:6.2f}, z = {:6.2f}'.format('gyro',padding, buf[0], buf[1], buf[2]))

            #print time data only if they are present in the frame received
            field_to_found = 'time'
            if(field_to_found in data):
                time = data[field_to_found]
                print('{:<{}} : {:.2f} seconds\n'.format('time',padding, time))

        except (serial_datagram.CRCMismatchError, serial_datagram.FrameError):
            sys.stderr.write("CRC error")


if __name__ == '__main__':
    main()
