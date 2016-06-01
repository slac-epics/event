#ifndef SY87739_H
#define SY87739_H

#include <inttypes.h>

/* Program for computing EVR PLL coefficients (MICREL SY87739)
 *
 * T. Straumann (strauman@slac.stanford.edu)
 * (some ideas borrowed from similar code by Jukka Pietarinen (MRF))
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t  Sy87739Num;
typedef uint64_t Sy87739Freq;

#define PRI_SY87739NUMd PRId8
#define PRI_SY87739NUMx PRIx8
#define PRI_SY87739NUMi PRIi8

#define PRI_SY87739FRQd PRId64

typedef uint32_t Sy87739CtlWrd;

typedef struct Sy87739ParmsRec_ {
	Sy87739Num p;
	Sy87739Num qp;
	Sy87739Num qpm1;
	Sy87739Num m;
	Sy87739Num n;
	Sy87739Num pp;
} Sy87739ParmsRec, *Sy87739Parms;

/* Control word MSB is never set, so we use that to flag
 * an error condition.
 */

#define SY87739_ERR                 0x80000000
#define SY87739_ERR_P    (SY87739_ERR | (1<<0))
#define SY87739_ERR_PP   (SY87739_ERR | (1<<1))
#define SY87739_ERR_M    (SY87739_ERR | (1<<2))
#define SY87739_ERR_N    (SY87739_ERR | (1<<3))
#define SY87739_ERR_QP   (SY87739_ERR | (1<<4))
#define SY87739_ERR_QPM1 (SY87739_ERR | (1<<5))

void
sy87739CtlWrd2Parms(Sy87739Parms p_p, Sy87739CtlWrd ctrl);

Sy87739CtlWrd
sy87739Parms2CtlWrd(Sy87739Parms p_p);

/* Returns ZERO on error (invalid parameters) */
Sy87739Freq
sy87739Parms2Freq(Sy87739Parms p_p, Sy87739Freq ref);

typedef enum { CLOSEST, UP, DOWN } SearchMode;

int
sy87739Freq2Parms(Sy87739Freq fdes, Sy87739Freq fref, SearchMode mode, Sy87739Parms parms, unsigned nParms);

int
sy87739CheckParms(FILE *errf, Sy87739Parms p_p);

void
sy87739DumpParms(FILE *f, Sy87739Parms p_p);

#ifdef __cplusplus
};
#endif

#endif
