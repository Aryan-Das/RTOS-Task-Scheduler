import serial
s = serial.Serial('/dev/tty.usbserial-BH00LY16', 9600, timeout=1)
while True:
    data = s.read(64)
    if data:
        print(data)