#include <stdio.h>
#include <inttypes.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sy87739.h>

/* Program for computing EVR PLL coefficients (MICREL SY87739)
 *
 * T. Straumann (strauman@slac.stanford.edu)
 * (some ideas borrowed from similar code by Jukka Pietarinen (MRF))
 */

typedef struct Sy87739SelRec_ {
	Sy87739Num psel;
	Sy87739Num qp;
	Sy87739Num qpm1;
	Sy87739Num msel;
	Sy87739Num nsel;
	Sy87739Num ppsel;
} Sy87739SelRec, *Sy87739Sel;

static Sy87739Num mntab[] = {
	16,16,18,17,31,14,32,15
};

static Sy87739Num sel_div(Sy87739Num sel)
{
	return sel+17;
}

static Sy87739Num sel_postdiv(Sy87739Num sel)
{
	if ( sel < 2 ) {
		return sel == 0 ? 1 : 3;
	} else if ( sel <=16 ) {
		return sel;
	} else if ( sel <= 32 ) {
		return (sel - 16)*2 + 16;
	} else {
		return (sel - 24)*4 + 32;
	}
}


static Sy87739Num find_mnsel(Sy87739Num mn)
{
Sy87739Num sel;
	for ( sel = 0; sel < sizeof(mntab)/sizeof(mntab[0]); sel++ ) {
		if ( mntab[sel] == mn ) {
			break;
		}
	}
	return sel;
}

static Sy87739Num find_postdivsel(Sy87739Num postdiv)
{
Sy87739Num postdivsel;

	for ( postdivsel = 0; postdivsel < 32; postdivsel++ ) {
		if ( sel_postdiv( postdivsel ) == postdiv )
			break;
	}
	return postdivsel;
}

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

Sy87739CtlWrd sy87739Parms2CtlWrd(Sy87739Parms p_p)
{
Sy87739CtlWrd ctrl = 0;
Sy87739Num    msel, nsel, postdivsel, divsel;

	if ( p_p->p < 17 || p_p->p > 32 ) {
		ctrl |= SY87739_ERR_P;
		divsel = 0; /* keep compiler happy */
	} else {
		divsel = p_p->p - 17;
	}

	if ( (postdivsel = find_postdivsel(p_p->pp)) >= 32 ) {
		ctrl |= SY87739_ERR_PP;
	}

	if ( (msel = find_mnsel(p_p->m)) > 7 ) {
		ctrl |= SY87739_ERR_M;
	}

	if ( (nsel = find_mnsel(p_p->n)) > 7 ) {
		ctrl |= SY87739_ERR_N;
	}

	if ( ! (ctrl & SY87739_ERR) ) {
		ctrl = p_p->qp;
		ctrl = (ctrl << 5) | p_p->qpm1;

		ctrl = (ctrl << 4) | divsel;
		ctrl = (ctrl << 8) | postdivsel;
		ctrl = (ctrl << 3) | nsel;
		ctrl = (ctrl << 3) | msel;
	}

	return ctrl;
}


static int mnLegal(Sy87739Num m, Sy87739Num n)
{
int rval = 0;
	if ( (m > 18) != (n > 18) ) {
		rval |= SY87739_ERR_N | SY87739_ERR_M;
	}

	if ( n * 14 >= m * 17 ) {
		rval |= SY87739_ERR_N | SY87739_ERR_M;
	}
	return rval;
}

static int qpsumLegal(Sy87739Num qpsum)
{
	return qpsum < 1 || qpsum > 31 ?  (SY87739_ERR_QP | SY87739_ERR_QPM1) : 0;
}

int sy87739CheckParms(FILE *errf, Sy87739Parms p_p)
{
int rval = 0;
int orval;
	
	if ( p_p->p < 17 || p_p->p > 32 ) {
		rval |= SY87739_ERR_P;
		if ( errf ) {
			fprintf(errf, "Parameter Error: P (%"PRI_SY87739NUMd" not in range 17..32\n", p_p->p);
		}
	}

	if ( p_p->qp > 31) {
		rval |= SY87739_ERR_QP;
		if ( errf ) {
			fprintf(errf, "Parameter Error: QP > 31\n");
		}
	}

	if ( p_p->qpm1 > 31 ) {
		rval |= SY87739_ERR_QPM1;
		if ( errf ) {
			fprintf(errf, "Parameter Error: QP-1 > 31\n");
		}
	}

	orval = rval;

	rval |= qpsumLegal(p_p->qp + p_p->qpm1);

	if ( errf && (orval & (SY87739_ERR_QP|SY87739_ERR_QPM1)) != (rval & (SY87739_ERR_QP|SY87739_ERR_QPM1)) ) {
		fprintf(errf, "Parameter Error: QP/QP-1 combination invalid\n");
	}

	if ( find_mnsel( p_p->m ) > 7 ) {
		rval |= SY87739_ERR_M;
		if ( errf ) {
			fprintf(errf, "Parameter Error: M invalid\n");
		}
	}

	if ( find_mnsel( p_p->n ) > 7 ) {
		rval |= SY87739_ERR_N;
		if ( errf ) {
			fprintf(errf, "Parameter Error: N invalid\n");
		}
	}

	orval = rval;

	rval |= mnLegal(p_p->m, p_p->n);

	if ( errf && (orval & (SY87739_ERR_M|SY87739_ERR_N)) != (rval & (SY87739_ERR_M|SY87739_ERR_N)) ) {
		fprintf(errf, "Parameter Error: M,N combination invalid\n");
	}

	if ( find_postdivsel(p_p->pp) > 31 ) {
		rval |= SY87739_ERR_PP;
		if ( errf ) {
			fprintf(errf, "Parameter Error: PP > 31\n");
		}
	}

	return rval;
}

static int selLegal(Sy87739Sel s_p)
{
int rval = 0;
Sy87739Num m = mntab[s_p->msel];
Sy87739Num n = mntab[s_p->nsel];


	rval = qpsumLegal( s_p->qp + s_p->qpm1 );

	rval |= mnLegal(m, n);

	return rval;
}

static void sel2parms(Sy87739Parms p_p, Sy87739Sel s_p)
{
	p_p->p    = sel_div( s_p->psel );
	p_p->qp   = s_p->qp;
	p_p->qpm1 = s_p->qpm1;
	p_p->m    = mntab[s_p->msel];
	p_p->n    = mntab[s_p->nsel];
	p_p->pp   = sel_postdiv( s_p->ppsel );
}

static void bfsel_init(Sy87739Sel s_p)
{
	s_p->psel  =  0;
	s_p->qp    =  0;
	s_p->qpm1  =  0;
	s_p->msel  =  1; /* msel == 0 redundant */
	s_p->nsel  =  1; /* nsel == 0 redundant */
	s_p->ppsel = -1;
}

typedef int (*Sy87739Cmp)(Sy87739Parms, void *);

static unsigned
gcf(unsigned a, unsigned b)
{
	while ( (a = a % b) != 0 ) {
		if ( 0 == (b = b % a) )
			return a;
	}
	return b;
}

static int bfsel(Sy87739Sel s_p, Sy87739Cmp cmp, void *closure)
{
int             rval;
Sy87739ParmsRec parms;

	sel2parms( &parms, s_p );

goto continue_here;  

	for ( ; s_p->psel < 16; s_p->psel++ ) {

		parms.p = sel_div( s_p->psel );

		for ( ; s_p->qp < 32; s_p->qp++ ) {

			parms.qp = s_p->qp;

			for ( ; s_p->qpm1 < 32; s_p->qpm1++ ) {

				Sy87739Num qpsum = s_p->qpm1 + s_p->qp;

				if ( 0 != qpsumLegal( qpsum ) )
					continue;

				if ( s_p->qpm1 > 0 ) {
					/* omit variants where qpm1 and (qpm1+qp) have a common factor */
				    if ( gcf( qpsum, s_p->qpm1 ) > 1)
						continue;
				} else {
					/* if qp-1 is zero then use only qp == 1 */
					if ( s_p->qp > 1 )
						continue;
				}

				parms.qpm1 = s_p->qpm1;

				for ( ; s_p->msel < 8; s_p->msel++ ) {

					parms.m = mntab[s_p->msel];

					for ( ; s_p->nsel < 8; s_p->nsel++ ) {

						parms.n = mntab[s_p->nsel];

						while ( s_p->ppsel < 32 ) {

							if ( 1 == s_p->ppsel )
								s_p->ppsel++; /* ppsel == 1 redundant */


							if ( 0 == selLegal(s_p) ) {

								parms.pp = sel_postdiv( s_p->ppsel );

								rval = cmp( &parms, closure );

								if ( rval )
									return rval;
							}
continue_here:
							s_p->ppsel++;
						}
						s_p->ppsel = 0;
					}
					s_p->nsel = 1;
				}
				s_p->msel = 1;
			}
			s_p->qpm1 = 0;
		}
		s_p->qp = 0;
	}

	return 0;
}

void sy87739CtlWrd2Parms(Sy87739Parms p_p, Sy87739CtlWrd ctrl)
{
Sy87739Num msel, nsel, postdivsel, divsel;

		msel       = (ctrl >>  0) & 7;
		nsel       = (ctrl >>  3) & 7;
		postdivsel = (ctrl >>  6) & 31;
		divsel     = (ctrl >> 14) & 15;

		p_p->qpm1  = (ctrl >> 18) & 31;
		p_p->qp    = (ctrl >> 23) & 31;

		p_p->pp    = sel_postdiv(postdivsel);

		p_p->m     = mntab[msel];
		p_p->n     = mntab[nsel];

		p_p->p     = sel_div(divsel);
}

uint64_t sy87739Parms2Den(Sy87739Parms p_p)
{
uint64_t qsum = p_p->qp + p_p->qpm1;
	return  p_p->m * qsum * p_p->pp;
}

uint64_t sy87739Parms2Num(Sy87739Parms p_p)
{
uint64_t qsum = p_p->qp + p_p->qpm1;
	return (p_p->p * qsum - p_p->qpm1) * p_p->n;
}

Sy87739Freq sy87739Parms2Freq(Sy87739Parms p_p, Sy87739Freq ref)
{
uint64_t num, den, qsum, rval, f;
	qsum = p_p->qp + p_p->qpm1;

	num = (p_p->p * qsum - p_p->qpm1) * p_p->n;
	den =  p_p->m * qsum * p_p->pp;

	rval = num*ref;
	rval = rval / den;

	f = rval * p_p->pp;

	if ( f < 540000000 || f > 729000000 )
		return 0; /* INVALID */

	return rval;
}

void sy87739DumpParms(FILE *f, Sy87739Parms p_p)
{
uint32_t ctrl;

	Sy87739Freq num = sy87739Parms2Num( p_p );
	Sy87739Freq den = sy87739Parms2Den( p_p );

	fprintf(f, "\n");
	fprintf(f, "%10"PRId64"\n", num);
	fprintf(f, "----------  = %g * Fref; @24MHz = %10"PRI_SY87739FRQd"Hz\n",
		(double)num/(double)den, sy87739Parms2Freq( p_p, 24000000 ));
	fprintf(f, "%10"PRId64"\n", den);

	fprintf(f, "\n");
	fprintf(f, "P   : %5"PRI_SY87739NUMd"\n", p_p->p);
	fprintf(f, "QP  : %5"PRI_SY87739NUMd"\n", p_p->qp);
	fprintf(f, "QP-1: %5"PRI_SY87739NUMd"\n", p_p->qpm1);
	fprintf(f, "M   : %5"PRI_SY87739NUMd"\n", p_p->m);
	fprintf(f, "N   : %5"PRI_SY87739NUMd"\n", p_p->n);
	fprintf(f, "PPST: %5"PRI_SY87739NUMd"\n", p_p->pp);

	ctrl = sy87739Parms2CtlWrd( p_p );

	if ( (SY87739_ERR & ctrl) ) {
		fprintf(stderr,"Invalid parameters: ");
		if ( (SY87739_ERR_P    & ctrl) )
			fprintf(stderr,"P ");
		if ( (SY87739_ERR_QP   & ctrl) )
			fprintf(stderr,"QP ");
		if ( (SY87739_ERR_QPM1 & ctrl) )
			fprintf(stderr,"QPM1 ");
		if ( (SY87739_ERR_M    & ctrl) )
			fprintf(stderr,"M ");
		if ( (SY87739_ERR_N    & ctrl) )
			fprintf(stderr,"N ");
		if ( (SY87739_ERR_PP   & ctrl) )
			fprintf(stderr,"PP ");
		fprintf(stderr,"\n");
	} else {
		fprintf(f, "\nControl word 0x%08"PRIx32"\n", ctrl);
		ctrl = ((ctrl >> 8) & 0x00ff00ff) | ((ctrl << 8) & 0xff00ff00);
		ctrl = ((ctrl >>16) & 0x0000ffff) | ((ctrl <<16) & 0xffff0000);
		fprintf(f, "Byte reverse 0x%08"PRIx32"\n", ctrl);
	}
}


typedef struct FreqsRec_ {
	Sy87739Freq fref, fdes, dopt;
	Sy87739ParmsRec         popt;
	SearchMode              mode;
} FreqsRec, *Freqs;

static int cmpFreq(Sy87739Parms p_p, void *closure)
{
Freqs       ctxt = (Freqs)closure;
Sy87739Freq f    = sy87739Parms2Freq( p_p, ctxt->fref );
Sy87739Freq diff;

	if ( f * p_p->pp < 540000000 || f * p_p->pp > 729000000 )
		return 0;

	if ( f > ctxt->fdes ) {
		if ( DOWN == ctxt->mode || ( 0 == (diff = f - ctxt->fdes) && UP   == ctxt->mode ) )
			return 0;
	} else {
		if ( UP   == ctxt->mode || ( 0 == (diff = ctxt->fdes - f) && DOWN == ctxt->mode ) )
			return 0;
	}

	if ( diff < ctxt->dopt ) {
		ctxt->dopt = diff;
		ctxt->popt = *p_p;
		return 1;
	} else if ( diff == ctxt->dopt ) {
		if (   ctxt->popt.p      == p_p->p
		    && ctxt->popt.qp     == p_p->qp
		    && ctxt->popt.qpm1   == p_p->qpm1
		    && ctxt->popt.pp     == p_p->pp ) {
			/* omit variants where m/n == 1 */
			if ( ctxt->popt.m == ctxt->popt.n && p_p->m == p_p->n )
				return 0;
		}
		ctxt->popt = *p_p;
		return 2;
	}
	return 0;
}

int
sy87739Freq2Parms(Sy87739Freq fdes, Sy87739Freq fref, SearchMode mode, Sy87739Parms parms, unsigned nParms)
{
Sy87739SelRec   it;
unsigned        nsols;
FreqsRec        freqs;
int             res;

	freqs.fref =  fref;
	freqs.dopt = -1;
	freqs.mode = mode;
	freqs.fdes = fdes;

	bfsel_init( &it );

	nsols = 0;

	while ( (res = bfsel( &it, cmpFreq, &freqs )) ) {
		if ( 1 == res ) {
			nsols = 0;
		}
		if ( nsols < nParms ) {
			parms[nsols] = freqs.popt;
		}
		nsols++;
	}

	return nsols;
}
