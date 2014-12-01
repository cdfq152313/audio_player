#include <stdint.h>
#include "hash-djb2.h"
#include "osdebug.h"

uint32_t hash_djb2(const uint8_t * str, uint32_t hash ,ssize_t _max) {
    uint32_t max = (uint32_t) _max;
    int c;
    
    //no initall hash number
    if(!hash){
        hash = 5381;
    }

    while (((c = *str++)) && max--) {
        hash = ((hash << 5) + hash) ^ c;
    }
    
    return hash;
}
