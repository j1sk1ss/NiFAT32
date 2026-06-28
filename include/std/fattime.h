#ifndef FATTIME_H_
#define FATTIME_H_

#ifndef NFT32_MAKE_TIME
    #define NFT32_MAKE_TIME(hour, minute, second) \
        ((unsigned short)( \
            (((unsigned short)((hour) & 0x1F)) << 11) | \
            (((unsigned short)((minute) & 0x3F)) << 5) | \
            ((unsigned short)(((second) / 2) & 0x1F)) \
        ))
#endif

#ifndef NFT32_MAKE_DATE
    #define NFT32_MAKE_DATE(year, month, day) \
        ((unsigned short)( \
            (((unsigned short)(((year) - 1980) & 0x7F)) << 9) | \
            (((unsigned short)((month) & 0x0F)) << 5) | \
            ((unsigned short)((day) & 0x1F)) \
        ))
#endif

#ifndef NFT32_MAKE_CREATION_TIME_TENTHS
    #define NFT32_MAKE_CREATION_TIME_TENTHS(second, centisecond) \
        ((unsigned char)( \
            (((second) & 1) * 100) + ((centisecond) & 0x7F) \
        ))
#endif

#endif