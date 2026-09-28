import time
import serial
import matplotlib.pyplot as plt

def read_value(ser):
    while True:
        try:
            line = ser.readline().decode('ascii').strip()
            t, p, h = map(float, line.split())
            return t, p, h
        except ValueError:
            continue

def main():
    ser = serial.Serial('COM14', 115200, timeout=1)

    if ser.is_open:
        print(f"Port {ser.name} opened")
    else:
        print(f"Failed to open port {ser.name}")
        return

    measure_t = []
    measure_p = []
    measure_h = []
    measure_ts = []

    start_ts = time.time()

    try:
        while True:
            ts = time.time() - start_ts

            temp_C, pressure_Pa, humidity = read_value(ser)

            measure_t.append(temp_C)
            measure_p.append(pressure_Pa)
            measure_h.append(humidity)
            measure_ts.append(ts)

            print(f'{temp_C:.2f} °C | {pressure_Pa:.0f} Pa | {humidity:.1f}% | {ts:.2f}s')

            time.sleep(0.1)

    except KeyboardInterrupt:
        print("\nStopped by user")

    finally:
        ser.close()
        print("Port closed")

        # 📊 ГРАФИКИ

        plt.figure(figsize=(8, 8))

        # Температура
        plt.subplot(3, 1, 1)
        plt.plot(measure_ts, measure_t)
        plt.title('Температура от времени')
        plt.ylabel('°C')
        plt.grid()

        # Давление
        plt.subplot(3, 1, 2)
        plt.plot(measure_ts, measure_p)
        plt.title('Давление от времени')
        plt.ylabel('Па')
        plt.grid()

        # Влажность
        plt.subplot(3, 1, 3)
        plt.plot(measure_ts, measure_h)
        plt.title('Влажность от времени')
        plt.xlabel('время, с')
        plt.ylabel('%')
        plt.grid()

        plt.tight_layout()
        plt.show()

if __name__ == "__main__":
    main()