program edefRelease	

%%#include <stdio.h>

#include "pvMacros.st"

int edefMaxNumber;
assign edefMaxNumber to "EVG:SYS0:1:EDEFMAXCOUNT";

int edefNumber;

string val;
assign val to "";
string pv;

entry
{
        delay_time = 0.03;

	PVGET (edefMaxNumber);

	edefNumber = 0;
}

ss edefReleaseStateSet
{
	state checking
	{	
	    when(delay(delay_time))
	    {
		edefNumber++;
		if (edefNumber > edefMaxNumber) edefNumber = 1;
		sprintf (pv, "EDEF:SYS0:%d:EDEFRELEASE", edefNumber);
		PVASSIGN (val, pv);
		PVGET (val);
		if (0 == strcmp(val, "ON"))
		{
		  sprintf (val, "OFF");
		  PVPUT (val);

		  sprintf (pv, "EDEF:SYS0:%d:CTRL", edefNumber);
		  PVASSIGN (val, pv);
		  PVPUT (val);

		  sprintf (val, "");
		  sprintf (pv, "EDEF:SYS0:%d:NAME", edefNumber);
		  PVASSIGN (val, pv);
		  PVPUT (val);

		  sprintf (pv, "EDEF:SYS0:%d:USERNAME", edefNumber);
		  PVASSIGN (val, pv);
		  PVPUT (val);
		}
	    } state checking
	}
}
