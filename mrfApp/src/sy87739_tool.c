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

static int getkey(char *key, int sz, FILE *f)
{
int i, ch;

	memset(key, 0, sz);

	for ( i=0; i<sz; i++ ) {
		if ( EOF == (ch = fgetc( f )) )
			return -1;
		if ( '\n' == (key[i] = ch) )
			return 1;
	}
	return 0;
}

static int parmsFromFile(FILE *f, Sy87739Parms p_p)
{
int      good = 0;
char     key[5];
uint64_t num;
int      got;

	do {

		if ( (got = getkey(key, sizeof(key), f)) < 0 )
			goto bail;

		if ( got > 0 )
			continue;

		if ( ':' == key[4] ) {
			if ( 1 != (got = fscanf(f, "%"SCNi64"\n", &num)) ) {
				if ( EOF == got ) {
					if ( ferror( f ) )
						perror("reading parameter file");
					goto bail;
				}
			} else {
				switch ( toupper((int)key[0]) ) {
					case 'P':
						if ( toupper((int)key[1]) == 'P' )
							p_p->pp = num;
						else
							p_p->p  = num;
						break;

					case 'Q':
						if ( '-' == key[2] )
							p_p->qpm1 = num;
						else
							p_p->qp   = num;
						break;

					case 'M':
						p_p->m = num;
						break;

					case 'N':
						p_p->n = num;
						break;

					default:
						fprintf(stderr,"Invalid Header\n");
						return -1;
				}
				good++;
			}
		} else {
			while ( '\n' != (got = fgetc(f)) ) {
				if ( EOF == got )
					goto bail;
			}
		}
	} while ( good < 6 );

	return sy87739CheckParms(stderr, p_p);

bail:
	return -1;
}

static void usage(const char *nm)
{
	fprintf(stderr, "Usage: %s [-h] [-c <ctrl_word>] [-b <ctrl_word_bytereversed>] [-f <freq>] [-d <freq>] [-u <freq>] [-n <max>] [-F <filename>]\n", nm);
	fprintf(stderr, "  -c <ctrl_word>     : translate ctrl_word into parameters and frequency\n");
	fprintf(stderr, "  -b <ctrl_word_rev> : byte-reverse and translate ctrl_word into parameters and frequency\n");
	fprintf(stderr, "  -f <freq>          : find closest frequency to 'freq'\n");
	fprintf(stderr, "  -u <freq>          : find closest frequency greater than  'freq'\n");
	fprintf(stderr, "  -d <freq>          : find closest frequency less than 'freq'\n");
	fprintf(stderr, "  -n <max>           : limit output to 'max' parameter sets\n");
	fprintf(stderr, "  -F <filename>      : read parameters from file\n");
	fprintf(stderr, "  -h                 : this message\n");
}


int
main(int argc, char **argv)
{
uint32_t ctrl;
int      opt;
int      rval = 1;
int      cw   = 0;

Sy87739Parms    sols    = 0;
unsigned        nsols   = 0;
uint32_t        maxsols = 4;
unsigned        i;
uint32_t       *i_p;
uint64_t       *l_p;
const char     *fnam    = 0;
FILE           *f       = 0;
int             stat;
Sy87739Freq     fdes    = 0;

Sy87739ParmsRec parms;
SearchMode      mode = CLOSEST;

	while ( (opt = getopt(argc, argv, "hc:b:f:u:d:n:F:")) > 0 ) {
		i_p = 0;
		l_p = 0;
		switch ( opt ) {
			case 'h':
				rval = 0;
			default:
				usage( argv[0] );
				return rval;

			case 'b': cw = -1; i_p = &ctrl; break;
			case 'c': cw = +1; i_p = &ctrl; break;

			case 'f': mode = CLOSEST; l_p = &fdes; break;
			case 'u': mode = UP;      l_p = &fdes; break;
			case 'd': mode = DOWN;    l_p = &fdes; break;

			case 'n': i_p = &maxsols;       break;

			case 'F': fnam = optarg;        break;
		}

		if ( i_p && 1 != sscanf(optarg,"%"SCNi32, i_p) ) {
			fprintf(stderr,"Unable to scan arg to option -%c\n", opt);
			return 1;
		}
		if ( l_p && 1 != sscanf(optarg,"%"SCNi64, l_p) ) {
			fprintf(stderr,"Unable to scan arg to option -%c\n", opt);
			return 1;
		}
	}

	if ( maxsols > 0 && fdes != 0 ) {

		sols = malloc( sizeof(*sols)*maxsols );
		if ( ! sols ) {
			fprintf(stderr,"No memory\n");
			return 1;
		}

		nsols = sy87739Freq2Parms(fdes, 24000000, mode, sols, maxsols);

		printf("Found %d solutions:\n", nsols);

		for ( i = 0; i<nsols; i++ ) {
			if ( i >= maxsols )
				break;
			sy87739DumpParms( stdout, &sols[i] );
		}

		free( sols );
	}


	if ( cw ) {
		if ( cw < 0 ) {
			ctrl = ((ctrl >> 8) & 0x00ff00ff) | ((ctrl << 8) & 0xff00ff00);
			ctrl = ((ctrl >>16) & 0x0000ffff) | ((ctrl <<16) & 0xffff0000);
		}

		sy87739CtlWrd2Parms( &parms, ctrl );

		if ( 0 == sy87739CheckParms( stderr, &parms ) )
			sy87739DumpParms( stdout, &parms );
	}

	if ( fnam ) {
		if ( ! (f = fopen(fnam, "r")) ) {
			perror("Unable to open file for reading");
			return 1;
		}

		do {
			stat = parmsFromFile(f, &parms);
			if ( 0 == stat )
				sy87739DumpParms( stdout, &parms );
		} while ( stat >= 0 );

		fclose( f );
		f = 0;
	}

	return 0;
}
