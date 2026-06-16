#ifndef ADDRESSES_H_
#define ADDRESSES_H_
#ifdef __cplusplus
extern "C" {
#endif

/* Journals placement */
#if !defined(JOURNAL_MULTIPLIER) && \
    !defined(GET_JOURNALSECTOR) 
    #define JOURNAL_MULTIPLIER       12983229U
    #define GET_JOURNALSECTOR(n, ts) (((((n) + 35) * JOURNAL_MULTIPLIER) >> 3) % (ts - 2))
#endif

/* Boot sectors placement */
#if !defined(BOOT_MULTIPLIER) && \
    !defined(GET_BOOTSECTOR)
    #define BOOT_MULTIPLIER          2654435761U // Knuth's multiplier (2^32 / φ)
    #define GET_BOOTSECTOR(n, ts)    (((((n) + 1) * BOOT_MULTIPLIER) >> 11) % (ts - 2))
#endif

/* File Allocation Tables placement */
#if !defined(FAT_MULTIPLIER) && \
    !defined(GET_FATSECTOR)
    #define FAT_MULTIPLIER           340573321U
    #define GET_FATSECTOR(n, ts)     (((((n) + 7) * FAT_MULTIPLIER) >> 13) % (ts - 23))
#endif 

/* Error sectors placement */
#if !defined(ERRORS_MULTIPLIER) && \
    !defined(GET_ERRORSSECTOR)
    #define ERRORS_MULTIPLIER        10986542U
    #define GET_ERRORSSECTOR(n, ts)  (((((n) + 23) * ERRORS_MULTIPLIER) >> 9) % (ts - 2))
#endif

/* Offset for a cluster */
#if !defined(CLUSTERS_MULTIPLIER) && \
    !defined(GET_CLUSTER_OFF)
    #define CLUSTERS_MULTIPLIER      3024953U
    #define GET_CLUSTER_OFF(n, tc)   (((((n) + 11) * CLUSTERS_MULTIPLIER) >> 1) % (tc))
#endif

#ifdef __cplusplus
}
#endif
#endif