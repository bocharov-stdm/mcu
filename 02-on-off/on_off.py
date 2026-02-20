import serial, sys, led


print('start')

serialName = "/dev/ttyACM0"  # Может отличаться на вашей машине!
serialBaudRate = 9600  # Возможно придётся менять
ser = serial.Serial(serialName, baudrate=serialBaudRate)

INSTRUCTION = "Type e to turn LED on, d to disable it. Type v for device info"
DEVICE = "nosok_pico"
VERSION = "2.2.8"


def help():
    ser.write(INSTRUCTION + "\n")

def device_info():
    ser.write(f"Device: {DEVICE}, ver: {VERSION}")

def led_en():
    led.led_enable()
    ser.write("LED on\n")

def led_dis():
    led.led_enable()
    ser.write("LED off\n")


FUNCS = {
    'e': led_en,
    'd': led_dis,
    'v': device_info,
    'h': help
}


try:
    while True:
        c = ser.read(1)
        if c == "q":
            break
        elif c not in FUNCS:
            help()
        else:
            f = FUNCS[c]
            f()
finally:
    ser.close()
    pass