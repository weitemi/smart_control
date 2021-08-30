import sys
import os
import struct

__version__ = "1.1"

if __name__=="__main__":

    ftype = {"mp3":0, "wav":1}

    #curent dirctionary
    file_list = os.listdir("./")
    #get the mp3 and wav file
    file_list = [x for x in file_list if ((x.endswith(".wav")) or (x.endswith(".mp3")))]
    fnum = len(file_list)

    #what is head?
    head_len = 8 + 64 * fnum
    
    print ('__version__: ', __version__)
    print ('file_list: ', file_list)
    print ('fnum: ', fnum)
    print ('header_len: ', head_len)
    print ('--------------------')
    print (' ')

    #define the audio_tone_uri floder
    tone_folder = "../../main"
    if not os.path.exists(tone_folder):
        os.makedirs(tone_folder)
    
    file_uri_h = "../../main/audio/audio_tone_uri.h"
    file_uri_c = "../../main/audio/audio_tone_uri.c"

    #define final bin file:audio-esp.bin
    bin_file = ''
    bin_file = struct.pack("<HHI",0x2053,fnum,0x0)
    
    #audio_tone_uri.c
    source_file = '#include "audio_tone_uri.h"\r\n\r\n'
    source_file += 'const char* tone_uri[]={\r\n\r\n'
    
    #mp3 bin format
    song_bin = ''
    
    cur_addr = head_len
    songIdx = 0
    for fname in file_list:
        #open mp3 file
        f = open(fname,'rb')
        print ('fname:', fname)
        
        #eg:fuck.mp3 -> mp3
        fileType = fname.split(".")[-1].lower()
        
        print ('song index: ', songIdx)
        print ('file type: ', fileType)
        
        # "flash://tone/0_fuck.mp3",
        source_file += '   ' + '"'+ "flash://tone/" + str(songIdx) + '_' + fname + '"'+ ',\r\n'

        #get the mp3 file size 
        songLen = os.path.getsize(fname)
        print ('file len: ', songLen)
        
        songAddr = cur_addr
        print ('songAddr: ', songAddr)
        RFU = [0]*12
        
        # build song's header
        print ('songLen: ', songLen)
        bin_file += struct.pack("<BBBBII"  ,0x28            #file tag
                                           ,songIdx         #song index
                                           ,ftype[fileType] #mp3->0
                                           ,0x0             #songVersion
                                           ,songAddr
                                           ,songLen          #song length
                                           )
        bin_file += struct.pack("<12I",*RFU)
        bin_file += struct.pack("<I",0x0)

        #read mp3 into binfile
        song_bin += f.read()
        songIdx += 1
        pad = '\xff'*((4-songLen%4)%4)
        song_bin += pad

        #update addr
        cur_addr += (songLen + ((4-songLen%4)%4))
        
        print ('bin len:',len(bin_file))
        print ('--------------------')
        print('')
        f.close()
        pass
    
    #add mp3 to binfile
    bin_file += song_bin

    #create audio-esp.bin and write data 
    f= open("audio-esp.bin", "wb")
    f.write(bin_file)
    f.close()      
    
    source_file += "};\r\n"
    
    #create audio_tone_uri.c
    f= open(file_uri_c, "wb")
    f.write(source_file)
    f.close() 

    #create audio_tone_uri.h
    include_file = '#ifndef __AUDIO_TONEURI_H__\r\n'+ '#define __AUDIO_TONEURI_H__\r\n\r\n'
    include_file += 'extern const char* tone_uri[];\r\n\r\n'
    include_file += 'typedef enum {\r\n'
    
    for fname in file_list:
        f = open(fname,'rb')
        print ('fname:', fname)
        include_file += '    ' + 'TONE_TYPE_' + fname.split(".")[0].upper() + ',\r\n'
        f.close()
        pass
        
        
    
    _end_str = """    TONE_TYPE_MAX,
} tone_type_t;

#endif 
"""
    include_file += _end_str
    f= open(file_uri_h, "wb")
    f.write(include_file)
    f.close()   

