#ifndef ADDRESSES_H_
#define ADDRESSES_H_
#ifdef __cplusplus
extern "C" {
#endif

static inline unsigned int mix32(unsigned int x) {
    x ^= x >> 16;
    x *= 0x7FEB352D;
    x ^= x >> 15;
    x *= 0x846CA68B;
    x ^= x >> 16;
    return x;
}

/* Type-specific seeds */
#define BOOT_SEED       0xA341316Cu
#define FAT_SEED        0xC8013EA4u
#define JOURNAL_SEED    0xAD90777Du
#define ERRORS_SEED     0x7E95761Eu
#define OFFSET_SEED     0xC07DEE11u

/* Journals placement */
#if !defined(GET_JOURNALSECTOR)
    #define GET_JOURNALSECTOR(n, ts) \
        (2u + (mix32((unsigned int)(n) + JOURNAL_SEED) % ((unsigned int)(ts) - 2u)))
#endif

/* Boot sectors placement */
#if !defined(GET_BOOTSECTOR)
    #define GET_BOOTSECTOR(n, ts) \
        (2u + (mix32((unsigned int)(n) + BOOT_SEED) % ((unsigned int)(ts) - 2u)))
#endif

/* File Allocation Tables placement */
#if !defined(GET_FATSECTOR)
    #define GET_FATSECTOR(n, ts) \
        (23u + (mix32((unsigned int)(n) + FAT_SEED) % ((unsigned int)(ts) - 23u)))
#endif

/* Error sectors placement */
#if !defined(GET_ERRORSSECTOR)
    #define GET_ERRORSSECTOR(n, ts) \
        (2u + (mix32((unsigned int)(n) + ERRORS_SEED) % ((unsigned int)(ts) - 2u)))
#endif

/* Error sectors placement */
#if !defined(GET_CLUSTER_OFF)
    #define GET_CLUSTER_OFF(n, ts) \
        (23u + (mix32((unsigned int)(n) + OFFSET_SEED) % ((unsigned int)(ts) - 23u)))
#endif

#ifdef __cplusplus
}
#endif
#endif