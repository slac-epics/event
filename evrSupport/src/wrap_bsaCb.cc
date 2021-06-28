#include <exception>
#include "bsaCallbackApi.h"
#include "wrap_bsaCb.h"


extern "C" {

void wrapBsaCb(BsaTimingCallback cb, void *pcbArg, BsaTimingData *pbsaData, unsigned long *cnt)
{
    try {
        cb(pcbArg, pbsaData);
    } catch (std::exception &e) {
        /* nothing todo */
        *cnt++;    // add exception counter
    }
}


} /* extern C */
