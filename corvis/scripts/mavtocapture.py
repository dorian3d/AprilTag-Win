#!/usr/bin/env python3
from PIL import Image
from struct import pack
from collections import defaultdict
import itertools
import json
import yaml
import csv
import sys
import math
from os import path

accel_type = 20
gyro_type = 21
image_raw_type = 29

# defined in rc_tracker.h
rc_IMAGE_GRAY8 = 0
rc_IMAGE_DEPTH16 = 1

try:
    input_dir, output_filename = sys.argv[1:3]
except Exception as e:
    print(e)
    print(sys.argv[0], "<MAV_folder> <output_filename>")
    sys.exit(1)

def load_body(base):
    with open(base + "/body.yaml") as f:
        return yaml.safe_load(f)

def load_sensor(base):
    with open(base + "/sensor.yaml") as f:
        return yaml.safe_load(f)

def load_imu_data(base):
    with open(base + "/data.csv") as f:
        header = f.readline()
        return [(t_ns,wx,wy,wz,ax,ay,az) for (t_ns,wx,wy,wz,ax,ay,az) in csv.reader(f)]

def load_cam_data(base):
    for i in itertools.count():
        with open(base + "/data.csv") as f:
            header = f.readline()
            return [(t_ns,base + "/data/" + filename) for (t_ns,filename) in csv.reader(f)]

def compute_extrinsics(m):
    assert m['rows'] == 4 and m['cols'] == 4
    import numpy as np
    from scipy import linalg
    G = np.array(m['data'])
    G.resize(m['rows'], m['cols'])
    W = linalg.logm(G[0:3,0:3])
    return {
        'T': list(G[0:3,3]),
        'W': [(W[2, 1] - W[1, 2])/2,
              (W[0, 2] - W[2, 0])/2,
              (W[1, 0] - W[0, 1])/2],
        'T_variance': [0,0,0],
        'W_variance': [0,0,0],
    }

s = load_body(input_dir)
cal = {'calibration_version': 10, 'device_id':"", 'device_type':s['comment'], 'cameras':[], 'imus':[], 'depths':[]}
data = []
for i in itertools.count():
    imu = input_dir + "/imu"+str(i)
    if not path.exists(imu):
        break
    s = load_sensor(imu)
    cal['imus'].append({
        'accelerometer': {
            'noise_variance': s[    'gyroscope_noise_density'] * s[    'gyroscope_noise_density'] * s['rate_hz'],
            'bias': [0,0,0],
            'bias_variance': [.02, .02, .02] ,
            'scale_and_alignment': [ 1,0,0, 0,1,0, 0,0,1 ],
        },
        'gyroscope':     {
            'noise_variance': s['accelerometer_noise_density'] * s['accelerometer_noise_density'] * s['rate_hz'],
            'bias': [0,0,0],
            'bias_variance': [1e-6,1e-6,1e-6],
            'scale_and_alignment': [ 1,0,0, 0,1,0, 0,0,1 ],
        },
        'extrinsics': compute_extrinsics(s['T_BS']),
    })
    for (t_ns,wx,wy,wz,ax,ay,az) in load_imu_data(imu):
        data.append({ 'ptype':gyro_type, 'id':i, 'time_ns':int(t_ns), 'w':tuple(map(float,(wx,wy,wz))) })
        data.append({ 'ptype':accel_type,'id':i, 'time_ns':int(t_ns), 'a':tuple(map(float,(ax,ay,az))) })

for i in itertools.count():
    cam = input_dir + "/cam"+str(i)
    if not path.exists(cam):
        break
    s = load_sensor(cam)
    cal['cameras'].append({
        'focal_length_px': [s['intrinsics'][0],s['intrinsics'][1]],
        'center_px': [s['intrinsics'][2],s['intrinsics'][3]],
        'size_px': s['resolution'],
        'distortion': { 'pinhole': { 'type': 'polynomial', 'k': s['distortion_coefficients'][0:3]  } }[s['camera_model']],
        'extrinsics': compute_extrinsics(s['T_BS']),
    })
    for (t_ns,name) in load_cam_data(cam):
        data.append({ 'ptype':image_raw_type,'id':i, 'time_ns':int(t_ns), 'name': name, 'rate_hz': float(s['rate_hz']) })


with open(output_filename + ".json", "w") as f:
  json.dump(cal, f, sort_keys=True,indent=4)

wrote_packets = defaultdict(int)
wrote_bytes = 0

with open(output_filename, "wb") as f:
  for p in sorted(data, key=lambda x: x['time_ns']):
    ptype, time_us, sensor_id = p['ptype'], int(p['time_ns']/1000), p['id']

    if ptype == image_raw_type:
        im = Image.open(p['name'])
        (w,h) = im.size
        b = 1
        stride = w*b
        d = im.tobytes()
        assert len(d) == stride*h
        data = pack('QHHHH', int(1000000/p['rate_hz']), w, h, stride, rc_IMAGE_GRAY8) + d
    elif ptype == gyro_type:
        data = pack('fff', *p['w'])
    elif ptype == accel_type:
        data = pack('fff', *p['a'])
    else:
        print("Unexpected data type", ptype)

    pbytes = len(data) + 16
    header_str = pack('IHHQ', pbytes, ptype, sensor_id, time_us)
    f.write(header_str)
    f.write(data)
    wrote_packets[ptype] += 1
    wrote_bytes += len(header_str) + len(data)
