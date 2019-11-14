# -*- coding: utf-8 -*-
"""
general functions and fitting.
(a half-decent language would have those functions already).
Python is worth what it costs.
"""

from itertools import product
import sys  # command line argument, print w/o newline, version
import numpy as np


NSmplRst = 2
NGrp = 212
NPad = 45
NADC = 7
NColInBlock = 32
NRowInBlock = NADC  # 7
NCol = NColInBlock * NPad  # 32*45=1440, including RefCol
NRow = NADC * NGrp  # 212*7=1484
NbitPerPix = 15

iGn = 0
iCrs = 1
iFn = 2
NGnCrsFn = 3
iSmpl = 0
#
ERRint16 = -256  # negative value usable to track Gn/Crs/Fn from missing pack
ERRDLSraw = 65535
# forbidden uint16, usable to track "pixel" from missing pack

# percival-specific data reordering functions

# from P2M manual
N_xcolArray = 4
N_ncolArray = 8
N_col_block = N_xcolArray * N_ncolArray
colArray = np.arange(N_col_block).reshape((N_xcolArray,
                                           N_ncolArray)).transpose()
colArray = np.fliplr(colArray)  # in the end it is colArray[ix,in]

# this gives the (iADC,iCol) indices of a pixel in a Rowblk,
# given its sequence in the streamout
ADCcolArray_1DA = []
for i_n, i_adc, i_x in product(range(N_ncolArray),
                               range(NADC)[::-1],
                               range(N_xcolArray)):
    ADCcolArray_1DA.append((i_adc, colArray[i_n, i_x]))

# to use this:  for ipix in range(32*7):
# (ord_ADC,ord_col)=ADCcolArray_1DA[ipix]
ADCcolArray_1DA = np.array(ADCcolArray_1DA)


iG = 0
iH0 = np.arange(21+1, 0+1-1, -1)
iH1 = np.arange(22+21+1, 22+0+1-1, -1)
iP2M_ColGrp = np.append(np.array(iG), iH0)
iP2M_ColGrp = np.append(iP2M_ColGrp, iH1)


def printcol(string, colour):
    ''' write in colour (red/green/orange/blue/purple) '''
    white = '\033[0m'  # white (normal)
    if (colour == 'black'):
        out_color = '\033[30m'  # black
    elif (colour == 'red'):
        out_color = '\033[31m'  # red
    elif (colour == 'green'):
        out_color = '\033[32m'  # green
    elif (colour == 'orange'):
        out_color = '\033[33m'  # orange
    elif (colour == 'blue'):
        out_color = '\033[34m'  # blue
    elif (colour == 'purple'):
        out_color = '\033[35m'  # purple
    else:
        out_color = '\033[30m'
    print(out_color+string+white)
    sys.stdout.flush()


def convert_hex_byteSwap_2nd(data2convert_Ar):
    ''' interpret the ints in an array as 16 bits.
        byte-swap them: (byte0,byte1) => (byte1,byte0)
    '''
    by0 = np.mod(data2convert_Ar, 2**8).astype('uint16')
    by1 = data2convert_Ar//(2**8)
    by1 = by1.astype('uint16')
    data_ByteSwapped_Ar = (by0 * (2**8)) + by1
    data_ByteSwapped_Ar = data_ByteSwapped_Ar.astype('uint16')
    return (data_ByteSwapped_Ar)


def convert_uint_2_bits_Ar(in_intAr, Nbits):
    ''' convert (numpyarray of uint => array of Nbits bits)
        for many bits in parallel
    '''
    inSize_T = in_intAr.shape
    in_intAr_flat = in_intAr.flatten()
    out_NbitAr = np.zeros((len(in_intAr_flat), Nbits), dtype=bool)
    for iBits in range(Nbits):
        out_NbitAr[:, iBits] = (in_intAr_flat >> iBits) & 1
    out_NbitAr = out_NbitAr.reshape(inSize_T+(Nbits,))
    return out_NbitAr


def convert_britishBits_Ar(british_bit_array):
    ''' Conversion of bits defined by RAL to normal bit Conversion:
            0=>1 , 1=>0

         Example:
            >>> convert_britishBis_Ar(1)
                0
    '''
    return 1 - british_bit_array



def convert_bits_2_uint16_Ar(bitarray):
    """ Convert (numpyarray of [... , ... , n_bits] to
        array of [... , ... ](int) """
    shape = bitarray.shape
    n_bits = shape[-1]
    out = np.zeros(shape[:-1], dtype='uint16')
    for ibit in range(n_bits):
        out = (out << 1) | bitarray[..., n_bits-ibit-1]
    return out


def decode_dataset_8bit(arr_in, bit_mask, bit_shift):
    """ Masks out bits and shifts """
    arr_out = np.bitwise_and(arr_in, bit_mask)
    arr_out = np.right_shift(arr_out, bit_shift)
    arr_out = arr_out.astype(np.uint8)
    return arr_out


def aggregate_to_GnCrsFn(raw_dset):
    """Extracts the coarse, fine and gain bits.
    0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
    -   B0  B1  F0  F1  F2  F3  F4  F5  F6  F7  C0  C1  C2  C3  C4
    """
    coarse_adc = decode_dataset_8bit(arr_in=raw_dset,
                                     bit_mask=0x1F,
                                     bit_shift=0)  # 0x1F -> 0000000000011111
    fine_adc = decode_dataset_8bit(arr_in=raw_dset,
                                   bit_mask=0x1FE0,
                                   bit_shift=5)  # 0x1FE0 -> 0001111111100000
    gain_bits = decode_dataset_8bit(arr_in=raw_dset,
                                    bit_mask=0x6000,
                                    bit_shift=5+8)  # 0x6000->0110000000000000
    return coarse_adc, fine_adc, gain_bits


# P2M specific descrambling
def reorder_pixels_GnCrsFn_par(disord_6DAr, NADC, NColInRowBlk):
    """ P2M pixel reorder for a image.
        (NImg,NSmplrst, NGrp,NDataPads,NADC*NColInRowBlk,3),disordered
          ((NImg,NSmplrst, NGrp,NDataPads,NADC,NColInRowBlk,3),ordered
    """
    (aux_NImg,
     aux_NSmplRst,
     aux_NGrp,
     aux_NPads,
     aux_pixInBlk,
     auxNGnCrsFn) = disord_6DAr.shape

    output_shape = (aux_NImg,
                    aux_NSmplRst,
                    aux_NGrp,
                    aux_NPads,
                    NADC,
                    NColInRowBlk,
                    auxNGnCrsFn)
    ord_7DAr = np.zeros(output_shape, dtype='uint8')
    aux_pixOrd_padDisord_7DAr = np.zeros(output_shape, dtype='uint8')
    # pixel reorder inside each block
    for ipix in range(NADC*NColInRowBlk):
        (ord_ADC, ord_Col) = ADCcolArray_1DA[ipix]
        aux_pixOrd_padDisord_7DAr[...,
                                  ord_ADC, ord_Col, :] = disord_6DAr[..., ipix, :]
    ord_7DAr[...] = aux_pixOrd_padDisord_7DAr[..., iP2M_ColGrp[:], :, :, :]
    return ord_7DAr


def descramble_to_crs_fn_gn(scrmblShot,
                            refColH1_0_Flag,
                            cleanMemFlag,
                            verboseFlag):
    """
    Descramble content of 1 odinDAQ-acquired (raw) shout.
    Return content in Gn/Crs/Fn format.

    Here is how data are scrambled:

    1a) the chips send data out interleaving RowGroups
        (7row x (32 Col x 45 pads) ) from Sample/Reset, as:
        Smpl, Smpl,   Smpl, Rst, Smpl, Rst, ... , Smpl, Rst,   Rst, Rst
    1b) the position of pixels (pix0/1/2 in the next point) is mapped to
        the (7row x 32 Col) block according to the adc_cols lost
    1c) inside a (7row x 32 Col) block, the data are streamed out as:
        bit0-of-pix0, bit0-of-pix1, bit0-of-pix2, ... , bit0-of-pix1, ...
    1d) all the data coming from the sensor are bit-inverted: 0->1; 1->0
    2a) the mezzanine takes the data coming from each (7 x 32) block,
        and add a "0" it in front of every 15 bits:
        xxx xxxx xxxx xxxx  yyy ... => 0xxx xxxx xxxx xxxx 0yyy ...
    2b) the mezzanine takes the data coming from a 44x (7 x 32) blocks
        (this corresponds to a complete RowCrp, ignoring RefCol )
        and interleaves 32 bits at a time:
        32b-from-pad1, 32b-from-pad2, ... , 32b-from-pad44,
        next-32b-from-pad1, next-32b-from-pad2, ...
    2c) the mezzanine packs the bits in Bytes, and the Bytes in UDPackets
        4 UDPackets contain all the bits of a 44x (7 x 32) Rowgrp.
        A complete image (Smpl+Rst) = 1696 UDPackets
        each UDPack has 4928Byte of information, and 112Byte of header.
        each UDPack is univocally identified by the header:
        - which Img (frame) the packet belongs to
        - datatype: whether it is part of a Smpl/Rst (respect. 1 or 0)
        - subframenumber (0 or 1)
        - packetnumber (0:423), which determines the RowGrp the
        packet's data goes into
        there are 1696 packets in an image; a packet is identified by the
        triplet (datatype,subframenumber,packetnumber)
    3a) the packets are sent from 2 mezzanine links (some on one link, some
        on the other), and are saved to 2 h5 files by odinDAQ
    4a) OdinDAQ byte-swaps every 16-bit sequence (A8 B8 becomes B8 A8)
    5a) OdinDAQ rearranges the 4 quarters of each rowblock
    6a) OdinDAQ seems to invert Smpl and Rst

    Args:
        dataSmpl_fl0,dataRst_fl0,dataSmpl_fl1,dataRst_fl1
        swapSmplRstFlag,seqModFlag, refColH1_0_Flag,verboseFlag,cleanMemFlag

    Returns:
        descrambled array (DLSraw format)
    """
    out_dscrmbl = np.copy(scrmblShot)
    scrmblShot = np.asarray(scrmblShot)
    NColInBlk = NColInBlock
    NDataPad = NPad - 1

    # convert to (1-img,row, col) because easier to integrate
    print(scrmblShot.shape)
    (auxNRow, auxNCol) = scrmblShot.shape
    scrmblShot = scrmblShot.reshape((1, auxNRow, auxNCol))
    NImg = 1

    # solving 4a DAQ-scrambling: byte swap in hex (By0,By1) => (By1,By0)
    # N.B.: possible swipe between Smpl and Rst
    if verboseFlag:
        printcol("solving DAQ-scrambling: byte-swapping and Smpl-Rst-swapping",
                 'blue')
    scrmblShot_byteSwap = convert_hex_byteSwap_2nd(scrmblShot)
    if cleanMemFlag:
        del scrmblShot

    # solving DAQ-scrambling: "pixel" reordering
    if verboseFlag:
        printcol("solving DAQ-scrambling: reordering subframes", 'blue')

    def convert_odin_daq_2_mezzanine(shot_in):
        ' descrambles the OdinDAQ-scrambling '
        (aux_n_img, aux_nrow, aux_ncol) = shot_in.shape
        aux_reord = shot_in.reshape((aux_n_img,
                                     NGrp,
                                     NADC,
                                     2,
                                     aux_ncol//2))
        aux_reord = np.transpose(aux_reord, (0, 1, 3, 2, 4))
        aux_reord = aux_reord.reshape((aux_n_img,
                                      NGrp,
                                      2,
                                      2,
                                      NADC*aux_ncol//4))
        aux_reordered = np.ones((aux_n_img,
                                 NGrp,
                                 4,
                                 NADC*aux_ncol//4), dtype='uint16') * ERRDLSraw
        aux_reordered[..., 0, :] = aux_reord[..., 0, 0, :]
        aux_reordered[..., 1, :] = aux_reord[..., 1, 0, :]
        aux_reordered[..., 2, :] = aux_reord[..., 0, 1, :]
        aux_reordered[..., 3, :] = aux_reord[..., 1, 1, :]
        aux_reordered = aux_reordered.reshape((aux_n_img,
                                               NGrp*NADC,
                                               aux_ncol))
        return aux_reordered

    data2srcmbl_noRefCol = np.ones((NImg,
                                    NSmplRst,
                                    NRow,
                                    auxNCol), dtype='uint16') * ERRDLSraw
    data2srcmbl_noRefCol[:, iSmpl, :, :] = convert_odin_daq_2_mezzanine(scrmblShot_byteSwap)
    if cleanMemFlag:
        del scrmblShot_byteSwap

    data2srcmbl_noRefCol = data2srcmbl_noRefCol.reshape((NImg,
                                                         NSmplRst,
                                                         NGrp,
                                                         NADC,
                                                         auxNCol))

    # track missing packets:
    # False==RowGrp OK; True== packet(s) missing makes rowgroup moot
    # (1111 1111 1111 1111 instead of 0xxx xxxx xxxx xxxx)
    missingRowGrp_Tracker = data2srcmbl_noRefCol[..., 0, 0] == ERRDLSraw
    # ---
    # descramble proper
    if verboseFlag:
        printcol("solving mezzanine&chip-scrambling: pixel descrambling",
                 'blue')
    multiImgWithRefCol = np.zeros((NImg,
                                   NSmplRst,
                                   NGrp,
                                   NPad,
                                   NADC*NColInBlk,
                                   NGnCrsFn), dtype='int16')
    #
    # refCol
    multiImgWithRefCol[..., 0, :, :] = ERRint16
    #
    # descrambling
    data2srcmbl_noRefCol = data2srcmbl_noRefCol.reshape((NImg,
                                                         NSmplRst,
                                                         NGrp,
                                                         NADC*auxNCol//(NDataPad*2),
                                                         NDataPad,
                                                         2))
    # 32bit = 2"pix" from 1st pad, 2"pix" from 2nd pad, etc.
    data2srcmbl_noRefCol = np.transpose(data2srcmbl_noRefCol,
                                        (0, 1, 2, 4, 3, 5)).reshape((NImg,
                                                                     NSmplRst,
                                                                     NGrp,
                                                                     NDataPad,
                                                                     NADC*auxNCol//NDataPad))
    # (NSmplRst,NGrp,NDataPad,NADC*aux_NCol//NDataPad)
    theseImg_bitted = convert_uint_2_bits_Ar(data2srcmbl_noRefCol, 16)[..., -2::-1].astype('uint8')
    # n_smplrst,n_grp,n_data_pads,n_adc*aux_ncol//n_data_pads,15bits
    if cleanMemFlag:
        del data2srcmbl_noRefCol
    theseImg_bitted = theseImg_bitted.reshape((NImg,
                                               NSmplRst,
                                               NGrp,
                                               NDataPad,
                                               NbitPerPix,
                                               NADC*NColInBlk))
    theseImg_bitted = np.transpose(theseImg_bitted, (0, 1, 2, 3, 5, 4))
    # (NImg, n_smplrst,n_grp,n_data_pads,NPixsInRowBlk,15)
    theseImg_bitted = convert_britishBits_Ar(theseImg_bitted).reshape((NImg,
                                                                       NSmplRst*NGrp*NDataPad*NADC*NColInBlk,
                                                                       NbitPerPix))

    theseImg_bitted = theseImg_bitted.reshape((NImg*NSmplRst*NGrp*NDataPad*NADC*NColInBlk,
                                               NbitPerPix))
    (aux_coarse, aux_fine, aux_gain) = aggregate_to_GnCrsFn(convert_bits_2_uint16_Ar(theseImg_bitted[:, ::-1]))
    multiImgWithRefCol[..., 1:, :, iGn] = aux_gain.reshape((NImg,
                                                            NSmplRst,
                                                            NGrp,
                                                            NDataPad,
                                                            NADC*NColInBlk))
    multiImgWithRefCol[..., 1:, :, iCrs] = aux_coarse.reshape((NImg,
                                                               NSmplRst,
                                                               NGrp,
                                                               NDataPad,
                                                               NADC*NColInBlk))
    multiImgWithRefCol[..., 1:, :, iFn] = aux_fine.reshape((NImg,
                                                            NSmplRst,
                                                            NGrp,
                                                            NDataPad,
                                                            NADC*NColInBlk))
    if cleanMemFlag:
        del aux_gain
        del aux_coarse
        del aux_fine
        del theseImg_bitted

    # reorder pixels and pads
    if verboseFlag:
        printcol("solving chip-scrambling: pixel reordering", 'blue')
    multiImg_Grp_dscrmbld = reorder_pixels_GnCrsFn_par(multiImgWithRefCol,
                                                       NADC,
                                                       NColInBlk)
    if cleanMemFlag:
        del multiImgWithRefCol

    # add error tracking
    if verboseFlag:
        printcol("lost packet tracking", 'blue')
    multiImg_Grp_dscrmbld = multiImg_Grp_dscrmbld.astype('int16')
    # -256 upto 255
    for iImg in range(NImg):
        for iGrp in range(NGrp):
            for iSmplRst in range(NSmplRst):
                if (missingRowGrp_Tracker[iImg, iSmplRst, iGrp]):
                    multiImg_Grp_dscrmbld[iImg, iSmplRst, iGrp, ...] = ERRint16

    # also err tracking for ref col
    multiImg_Grp_dscrmbld[..., 0, :, :, :] = ERRint16
    if refColH1_0_Flag:
        if verboseFlag:
            printcol("moving RefCol data to G", 'blue')
        multiImg_Grp_dscrmbld[..., 0, :, :, :] = multiImg_Grp_dscrmbld[..., 44, :, :, :]
        multiImg_Grp_dscrmbld[..., 44, :, :, :] = ERRint16
    # ---
    #
    # reshaping as an Img array: (NImg,Smpl/Rst,n_grp,n_adc,n_pad,NColInBlk,Gn/Crs/Fn)
    if verboseFlag:
        printcol("reshaping as an Img array", 'blue')
    dscrmbld_GnCrsFn = np.transpose(multiImg_Grp_dscrmbld,
                                    (0, 1, 2, 4, 3, 5, 6)).astype('int16').reshape((NImg,
                                                                                    NSmplRst,
                                                                                    NGrp*NADC,
                                                                                    NPad*NColInBlk,
                                                                                    NGnCrsFn))
    if cleanMemFlag:
        del multiImg_Grp_dscrmbld
    # that's all folks
#    out_dscrmbl = dscrmbld_GnCrsFn[0, 0, :, 32:, iCrs].astype('uint16')
#    return out_dscrmbl

    coarse = dscrmbld_GnCrsFn[0, 0, :, 32:, iCrs].astype('uint16')
    fine = dscrmbld_GnCrsFn[0, 0, :, 32:, iFn].astype('uint16')
    gain = dscrmbld_GnCrsFn[0, 0, :, 32:, iGn].astype('uint16')

    return (coarse, fine, gain)
