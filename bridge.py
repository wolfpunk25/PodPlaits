#!/usr/bin/env python3
"""USB serial relay. Standard Python 3 only; no packages required."""
import argparse
import glob
import os
import re
import select
import sys
import termios
import time

def valid_state(line):
    fields=line.split()
    if len(fields)!=16 or fields[0]!=b"PSTATE" or not all(v.isdigit() for v in fields[1:]):return False
    v=list(map(int,fields[1:]))
    return 0<=v[0]<24 and all(0<=x<=1000 for x in v[1:10]) and v[10] in (0,1) and v[11] in (0,1) and 0<=v[12]<=127

def valid_name_hex(value):
    if not re.fullmatch(rb"(?:[0-9A-Fa-f]{2}){1,20}",value):return False
    name=bytes.fromhex(value.decode())
    return all(32<=c<=126 for c in name) and bool(name.strip())

def valid_source(line):
    fields=line.split()
    if len(fields)!=7 or fields[0]!=b"SCENE" or not all(v.isdigit() for v in fields[1:6]):return False
    slot,revision,status,exists,dirty=map(int,fields[1:6])
    return slot<32 and revision<=0xffffffff and status<8 and exists in (0,1) and dirty in (0,1) and valid_name_hex(fields[6])

def valid_command(line):
    m=re.fullmatch(rb"MOOD ([0-5])",line)
    if m:return True
    m=re.fullmatch(rb"SCENE (?:SELECT|LOAD) (\d{1,2})",line)
    if m:return int(m[1])<32
    m=re.fullmatch(rb"SCENE SAVE (\d{1,2}) ((?:[0-9A-Fa-f]{2}){1,20})",line)
    if m:return int(m[1])<32 and valid_name_hex(m[2])
    if line in (b"PLAY 0",b"PLAY 1",b"MODE 0",b"MODE 1"):return True
    m=re.fullmatch(rb"SET ([0-8]) (\d{1,4})",line)
    if m:return int(m[2])<=1000
    m=re.fullmatch(rb"MODEL (\d{1,2})",line)
    return bool(m and int(m[1])<24)

def open_port(path):
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[4] = attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd

def choose(pattern, description):
    paths = glob.glob(pattern)
    if len(paths) != 1:
        raise RuntimeError(f"Cannot identify {description}: {paths}. Use --pod and --display to specify ports.")
    return paths[0]

def run(pod_path, display_path):
    pod = open_port(pod_path)
    try:
        display = open_port(display_path)
    except BaseException:
        os.close(pod)
        raise
    buffers = {pod: bytearray(), display: bytearray()}
    pending = {pod: bytearray(), display: bytearray()}
    last_state = time.monotonic()
    connected = False
    print(f"Pod: {pod_path}\nDisplay: {display_path}\nWaiting for Pod settings…", flush=True)
    try:
        while True:
            readable, writable, _ = select.select(list(buffers), [fd for fd, b in pending.items() if b], [], .25)
            for fd in readable:
                chunk = os.read(fd, 4096)
                if not chunk:
                    raise OSError("A USB connection closed")
                buffers[fd].extend(chunk)
                while b"\n" in buffers[fd]:
                    raw, _, remainder = buffers[fd].partition(b"\n")
                    buffers[fd] = bytearray(remainder)
                    line = bytes(raw).rstrip(b"\r")
                    if fd == pod and valid_state(line):
                        last_state = time.monotonic()
                        if not connected:
                            print("Pod connected. Touchscreen controls are ready.", flush=True)
                            connected = True
                        if len(pending[display]) < 4096:
                            pending[display].extend(line + b"\n")
                    elif fd == pod and valid_source(line):
                        if len(pending[display]) < 4096:
                            pending[display].extend(line + b"\n")
                    elif fd == display and connected and valid_command(line):
                        if len(pending[pod]) < 4096:
                            pending[pod].extend(line + b"\n")
                if len(buffers[fd]) > 512:
                    buffers[fd].clear()
            for fd in writable:
                try:
                    n = os.write(fd, pending[fd])
                    del pending[fd][:n]
                except BlockingIOError:
                    pass
            if connected and time.monotonic() - last_state > 2:
                connected = False
                pending[pod].clear()
                print("Pod settings stopped arriving; check its USB cable.", flush=True)
    finally:
        os.close(pod)
        os.close(display)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pod")
    parser.add_argument("--display")
    args = parser.parse_args()
    try:
        pod_path = args.pod or choose("/dev/cu.usbmodem374B34773233*", "Pod")
        display_path = args.display or choose("/dev/cu.usbserial-*", "display")
        while True:
            try:
                run(pod_path, display_path)
            except (OSError, termios.error) as exc:
                print(f"USB connection interrupted: {exc}. Retrying in two seconds…", flush=True)
                time.sleep(2)
    except KeyboardInterrupt:
        print("\nRelay stopped.")
    except RuntimeError as exc:
        print(f"Relay stopped: {exc}", file=sys.stderr)
        sys.exit(1)
