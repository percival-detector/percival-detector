try:
    from pkg_resources import require
    require('h5py')
except:
    pass

import numpy as np
import h5py
import argparse

def decode_dataset_8bit( arr_in, bit_mask, bit_shift ):
    arr_out = np.bitwise_and( arr_in, bit_mask)
    arr_out = np.right_shift( arr_out, bit_shift )
    arr_out = arr_out.astype(np.uint8)
    return arr_out
    
def main():
    parser = argparse.ArgumentParser(description="""Decode fine/coarse ADC and gain bits from Percival data.
    
    The resulting datasets are stored in the input file.
    """)
    parser.add_argument("file", help="HDF5 file with Percival datasets to decode")
    args = parser.parse_args()
    
    fname = args.file
    print fname

    with h5py.File(fname, 'r+', libver='latest') as f:
        print f.items()
        
        for name, raw_dset in f.items():  
            name = name.strip('/')          
            print "decoding: " + name
            fine_adc = decode_dataset_8bit( raw_dset, 0x03FC, 2 )
            print fine_adc.shape
            f.create_dataset("fine_adc_"+name, data = fine_adc)
            coarse_adc = decode_dataset_8bit( raw_dset, 0x7C00, 2+8)
            f.create_dataset("coarse_adc_"+name, data = coarse_adc)
            gain_bits = decode_dataset_8bit( raw_dset, 0x0003, 0)
            f.create_dataset("gain_"+name, data = gain_bits)
            
if __name__=="__main__":
    main()
    
