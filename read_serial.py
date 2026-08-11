import serial, time

s = serial.Serial('/dev/tty.usbserial-BH00LY16', 9600, timeout=1)
last = time.time()
while True:
    line = s.readline()
    if line:
        now = time.time()
        print(f"[+{now - last:.3f}s] {line.decode(errors='replace')}", end='')
        last = now