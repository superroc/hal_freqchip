import os, sys
import struct
import zlib

CHIP_TYPE_FR303x = 0
CHIP_TYPE_FR509x = 1

CHIP_TYPE = CHIP_TYPE_FR509x

UNPROTECT = 1                   # unprotect enable, for OTA
VOLATILE_MODE = 1               # volatile mode enable when unprotect flash, for OTA
UNPROTECT_BITS = 0x00           # protect bits setting when unprotect flash, for OTA
DEAL_CMP_BIT = 1                # deal cmp bit when unprotect flash, for OTA
CMP_BIT_POS = 6                 # cmp bit position in status-2, for OTA
WRITE_STATUS_2_SEPERATE = 1     # write status-2 with 0x31(1) or 0x01(0) when unprotect flash, for OTA
ENABLE_CACHE_FOR_B = 0          # enable cache in ota procedure, for OTA
QSPI_DIVIDOR = 1                # qspi dividor
WRITE_TYPE = 0                  # write type
READ_TYPE = 2                   # read type
OVERLAP = 1                     # execute and store zone of B is overlap or not, for OTA
ENABLE_CACHE_FOR_U = 1          # enable cache before enter user code
CMP_BIT_SET = 0                 # cmp bit setting when unprotect flash, for OTA
REPROTECT = 0                   # reset protect bits after OTA is finished, for OTA

def fill_header(f_in_file, 
                    f_out_file, 
                    store_offset,
                    version = 0xffffffff,
                    exec_offset = 0x2000,
                    copy_unit = 16*1024,
                    copy_flag_store_step = 32):
    info_crc = 0xffffffff
    code_length = os.path.getsize(f_in_file)
    f_out = open(f_out_file, 'wb')
    f_out.write(struct.pack('I', version))
    f_out.write(struct.pack('I', store_offset))
    f_out.write(struct.pack('I', code_length))
    f_out.write(struct.pack('I', exec_offset))
    f_out.write(struct.pack('I', copy_unit))
    f_out.write(struct.pack('I', copy_flag_store_step))
    f_in = open(f_in_file, 'rb')
    array = f_in.read(code_length)
    image_crc = zlib.crc32(array)
    
    f_out.write(struct.pack('B', image_crc&0xff))
    f_out.write(struct.pack('B', (image_crc&0xff00)>>8))    
    f_out.write(struct.pack('B', (image_crc&0xff0000)>>16))    
    f_out.write(struct.pack('B', (image_crc&0xff000000)>>24))
    
    #f_out.write(struct.pack('I', image_crc))
    f_out.write(struct.pack('I', 0x51525251))
    if CHIP_TYPE == CHIP_TYPE_FR509x:
        image_tlv_length = 0
        f_out.write(struct.pack('I', image_tlv_length))
    options = (UNPROTECT<<0)        \
                | (VOLATILE_MODE<<1)    \
                | (UNPROTECT_BITS<<2) \
                | (DEAL_CMP_BIT<<7)    \
                | (CMP_BIT_POS<<8)    \
                | (WRITE_STATUS_2_SEPERATE<<11)   \
                | (ENABLE_CACHE_FOR_B<<12)   \
                | (QSPI_DIVIDOR<<13)   \
                | (WRITE_TYPE<<17)   \
                | (READ_TYPE<<19)   \
                | (OVERLAP<<22)     \
                | (ENABLE_CACHE_FOR_U<<23)  \
                | (CMP_BIT_SET<<24) \
                | (REPROTECT<<25)
    f_out.write(struct.pack('I', options))
    f_out.write(struct.pack('I', info_crc))
    f_out.write(struct.pack('I', 0x51525251))
    first_sector_last_size = 0
    if CHIP_TYPE == CHIP_TYPE_FR509x:
        first_sector_last_size = 0x1000-12*4
    else:
        first_sector_last_size = 0x1000-4-9*4
    for i in range(first_sector_last_size):
        f_out.write(struct.pack('B', 0xff))
    f_out.seek(0x2000)
    f_out.write(array)
    f_out.close()

if __name__ == '__main__':
    '''
    argv[1]: project name
    argv[2]: output path
    '''

    input_bin_name = os.path.join(sys.argv[2], sys.argv[1] + ".bin")
    if os.path.exists(input_bin_name):
        output_bin_name = os.path.join(sys.argv[2], sys.argv[1] + "_burn.bin")
        fill_header(input_bin_name, output_bin_name, 0)
        print("program target with file %s" % output_bin_name)
    else:
        print("INVALID INPUT PARAMTER for python script")
