#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include <stdlib.h>
#include <string.h>
struct nuqthdr_s;
struct nuqtdim_s;

static __used__ void bark_noise_hybridmp(int, abi_long const *, float const *, float *, float, int) {
}

extern "C" {

    void *OggAllocMem(u32 bytes) {
        void *memory = malloc(bytes);
        memset(memory, 0, bytes);
        return memory;
    }

    void OggFreeMem(void *memory) {
        free(memory);
    }

    void *OggReAllocMem(void *memory, u32 bytes) {
        return realloc(memory, bytes);
    }

} // extern "C"
