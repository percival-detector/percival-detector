#!/dls_sw/apps/python/anaconda/1.7.0/64/bin/python

import time
import os
import re
import struct
import re;
import h5py;
import numpy as np;

# This is a hacky python script to check the output from the FileWriter
#  in the FP is what you would expect after using the
#  PercivalFrameSimulator.
# You must alter the filename, dataset, fillmode and frame-num
#  as appropriate.
# This file is liable to modification.

f = h5py.File('/tmp/haha_000001.h5','r')

d = f["data"].value;

packet_fill_mode = 1;

frame_to_check = 1;
fr = d[frame_to_check,:,:];

pkqty = 4928/2;

i = 0;
# these two need to be persistent, so we initialize to a random number
counter = firstpixinpacket = 5;

while i < 1484 * 1408:
    row = i / 1408;
    col = i % 1408;
    pkt = i / pkqty;
    pktp = i % pkqty;

    if packet_fill_mode==0:
        if (pktp == 0):
            firstpixinpacket = fr[row,col];
            if( firstpixinpacket==pkt ):
                print "frame %d, pkti %d is good" % (frame_to_check,pkt);
            elif firstpixinpacket==0:
                print "pkti %d was dropped" % pkt;
            else:
                print "pkti %d has bad first pixel: val %d" % (pkt, firstpixinpacket); 
        else:
            if (fr[row,col] != firstpixinpacket):
                print "pixel bad at ", row, col;

    elif packet_fill_mode==1:

        if (pktp == 0):
            if(pkt == 0):
                counter = 0;
            firstpixinpacket = fr[row,col];
            if( firstpixinpacket == counter ):
                print "frame %d, pkti %d is good" % (frame_to_check, pkt);
            elif firstpixinpacket==0:
                print "pkti %d was dropped" % pkt;
            else:
                print "pkti %d has bad first pixel: val %d" % (pkt, firstpixinpacket); 
        else:
            if (fr[row,col]!=0 and fr[row,col] != counter):
                print "pixel bad at ", row, col;

        counter = counter + 1;
        if(counter == 65536):
            counter = 0;
        
    i = i+1;

