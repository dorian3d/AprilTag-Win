#!/usr/bin/env python
from __future__ import print_function
from packet import Packet, PacketType
import sys

if len(sys.argv) != 3:
    print("Usage:", sys.argv[0], "<capture file> <json file>")
    sys.exit(1)

filename = sys.argv[1]
json_cal = open(sys.argv[2]).read()

f = open(filename, "rb")

p = Packet.from_file(f)
while p is not None:
    if p.header.type == PacketType.calibration_json:
        if p.data == json_cal:
            print("Calibrations are the same")
        else:
            print("Calibrations are different")
        sys.exit(0)

    p = Packet.from_file(f)

f.close()

print("Error: No packet_calibration_json found!")
sys.exit(1)
