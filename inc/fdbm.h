#ifndef __HEADER_FDBM__
#define __HEADER_FDBM__

// Features: swp half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpv4 idiva idivt

void applyFDBM_simple1(char* buffer, int size, int doa);

void applyFDBM_simple2(char* buffer, int size, int doa1, int doa2);

void applyFDBM(char* buffer, int size, const int const * doa, int sd);

#endif /* end of include guard: __HEADER_FDBM__ */
