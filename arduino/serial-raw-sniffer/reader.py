#!/usr/bin/env python

# 
# MIT License
# 
# Copyright (c) 2016, 2017 Kent A. Vander Velden
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# 
# Kent A. Vander Velden
# kent.vandervelden@gmail.com
# Originally written August 23, 2017

import sys
import serial
import time
import subprocess


serial_port = '/dev/cu.usbmodem141101'


def open_serial():
    ser = serial.Serial(port=serial_port, baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=1, xonxoff=False, rtscts=False, dsrdtr=False) #, write_timeout=None, dsrdtr=False) #, inter_byte_timeout=None)

    #ser.reset_input_buffer()
    #ser.reset_output_buffer()
    ser.flushInput()
    ser.flushOutput()

    # clear input (what do the previous flush command actually do?)

    return ser


def strip_timestamp(ln):
    try:
        a, b = ln.split(':', 1)
        a1, a2 = a.split(' ', 1)
        ln = a1 + ':' + b
    except:
        pass

    return ln


def main():
    ser = open_serial()

    clearing = True
    frames = []
    buf = []
    ln = ''
    while True:
        v = ser.read(1)
        ln += v
        if v == '\n':
            ln = ln.rstrip()
            if ln != '':
                buf += [ln]
                if not clearing:
                    print 'Frame{0:03d}'.format(len(frames)), ln
                else:
                    print 'clearing', ln
            else:
                print '-' * 60
                if not clearing:
                    if len(buf) > 0:
                        frames += [buf]
                buf = []
                if clearing:
                    ln = ''
                    clearing = False
                    continue

                if len(frames) >= 2:
                    def h(fn, buf, remove_timestamp):
                        f = open(fn, 'w')
                        for ln in buf:
                            if remove_timestamp:
                                print >> f, strip_timestamp(ln)
                            else:
                                print >> f, ln

                    fn1 = 'frame{0:03d}_ts.txt'.format(len(frames) - 2)
                    fn2 = 'frame{0:03d}_ts.txt'.format(len(frames) - 1)
                    h(fn1, frames[-2], False)
                    h(fn2, frames[-1], False)

                    fn1 = 'frame{0:03d}.txt'.format(len(frames) - 2)
                    fn2 = 'frame{0:03d}.txt'.format(len(frames) - 1)
                    h(fn1, frames[-2], True)
                    h(fn2, frames[-1], True)
                    # subprocess.Popen(["meld", fn1, fn2]) 
            ln = ''


if __name__ == "__main__":
    main()
