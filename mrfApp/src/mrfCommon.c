/*
 *	Common mrfApp EVR driver code
 *
 * Copyright 2014, Stanford University
 * Authors:
 *		Bruce Hill <bhill@slac.stanford.edu>
 *
 * Released under the GPLv2 licence <http://www.gnu.org/licenses/gpl-2.0.html>
 */

#include "drvMrfEr.h"
#include "mrfCommon.h"

/**************************************************************************************************/
/*                              Private APIs                                                      */
/*                                                                                                */

char *	FormFactorToString( int formFactor )
{
	char	*	pString;
	switch ( formFactor )
	{
	default:		pString	= "Invalid";	break;
	case PMC_EVR:	pString	= "PMC_EVR";	break;
	case CPCI_EVR:	pString	= "cPCI_EVR";	break;
	case VME_EVR:	pString	= "VME_EVR";	break;
	case SLAC_EVR:	pString	= "SLAC_EVR";	break;
	case PCIE_EVR:	pString = "PCIE_EVR";	break;
	}
	return pString;
}


int FpgaVersionToFormFactor( epicsUInt32 fpgaVersion )
{
	int		formFactor;
	int		id = (fpgaVersion >> 24) & 0x0F;
	switch ( id )
	{
	case 0x1:	formFactor	= PMC_EVR;	break;
	case 0x0:	formFactor	= CPCI_EVR;	break;
	case 0x2:	formFactor	= VME_EVR;	break;
	case 0x7:	formFactor	= PCIE_EVR;	break;
	case 0xf:	formFactor	= SLAC_EVR;	break;
	default:	formFactor	= -1;		break;
	}
	return formFactor;
}


int ErGetFormFactor( ErCardStruct	*	pCard )
{
	if ( pCard == NULL )
	{
		printf( "ErGetFormFactor: Error, NULL pCard\n" );
		return -1;
	}

	return FpgaVersionToFormFactor( ErGetFpgaVersion(pCard) );
}

