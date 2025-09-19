"""

This script reads and saves messages sent by RX via serial port.

It is fairly quiet, so if you want to see messages in
real-time, you need to use an editor that allows refreshing
from the end of the file, for example `less`:

    less +F lora-receive.jsonl

usage:

    python serial_read.py # Default port
    python serial_read.py --port /dev/cu.usbmodem311101 # Custom port

Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com

"""

import serial
import serial.tools.list_ports
import threading
from rich import print
import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument(
    "--port", default="/dev/cu.usbmodem311101", help="Serial port to connect to"
)
args = parser.parse_args()
PORT = args.port
BAUD_RATE = 115200
FILENAME = "lora-receive.jsonl"
ERR_COUNT = 0


def test_port_availability(port):
    """___"""
    ports = [port.device for port in serial.tools.list_ports.comports()]
    port_available = port in ports
    return port_available


def read_serial(ser, stop_event):
    """___"""
    error_logged = False
    while not stop_event.is_set():
        try:
            data = ser.readline().decode("utf-8")
            if data:
                write_to_file(data)
                error_logged = False  # Reset error flag on successful read
        except Exception as _e:
            if not error_logged:
                error_msg = f"\n# ERROR IN {__file__}: {_e}\n"
                write_to_file(error_msg)
                print(f"Serial error: {_e}")
                error_logged = True
            # Wait a bit before retrying to avoid busy loop
            stop_event.wait(1.0)


def write_to_file(data):
    """___"""
    try:
        json.loads(data)
        jsonl_data = data.strip()
    except json.JSONDecodeError as _e:
        global ERR_COUNT
        ERR_COUNT += 1
        print("======== JSONDecodeError")
        print(_e)
        print(data)
        print(jsonl_data)
        print("=======/ JSONDecodeError\n")
        jsonl_data = f'{{"#":{ERR_COUNT},"msg":"{_e}"}}'

    with open(FILENAME, "a") as file:
        file.write(jsonl_data + "\n")
        file.flush()


def main():
    """___"""
    try:
        if test_port_availability(PORT):
            print(f"\n✅ {PORT}")
        else:
            raise serial.SerialException(f"Port {PORT} is not available")

        ser = serial.Serial(PORT, BAUD_RATE, timeout=1)
    except serial.SerialException as e:
        print(f"❌ Serial error: {e}")
        return

    stop_event = threading.Event()
    read_thread = threading.Thread(target=read_serial, args=(ser, stop_event))
    read_thread.start()

    try:
        while True:
            ans = input()
            if ans in ["q"]:
                break
    except KeyboardInterrupt:
        pass
    finally:
        stop_event.set()
        read_thread.join()
        ser.close()


def splash_screen():
    """___"""
    print(f"\nMonitor with:\nless +F {FILENAME}")


if __name__ == "__main__":
    splash_screen()
    main()
