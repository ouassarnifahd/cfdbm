#ifndef __HEADER_FDBM__
#define __HEADER_FDBM__

#define DOA_NOT_INITIALISED (1ull<<6)

#define DOA_LEFT   (-70l)
#define DOA_CENTER (  0l)
#define DOA_RIGHT  ( 70l)

void applyFDBM_simple1(char* buffer, size_t size, int doa);

void applyFDBM_simple2(char* buffer, size_t size, int doa1, int doa2);

void applyFDBM(char* buffer, size_t size, const int const * doa, int sd);

// Features: swp half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpv4 idiva idivt

#endif /* end of include guard: __HEADER_FDBM__ */
