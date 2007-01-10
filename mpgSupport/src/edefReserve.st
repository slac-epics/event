program edefReserve

option -a;  /* All pvGets must be synchronous */
option +c;  /* All connections must be made before execution begins */
option +d;  /* Turn on runtime debug messages */

%%#include <stdio.h>

/* {} Note: I probably shouldn't hard code "SYS0", but pass it in as a macro - MIke Zelazny 10-Jan-2007 */

string  edefRequestName;
assign  edefRequestName to "IOC:SYS0:1:EDEFNAME";
monitor edefRequestName;
evflag  edefRequestName_ef;
sync    edefRequestName edefRequestName_ef;

int edefMaxNumber;
int edefNumber;
int edefReserveAttempt;
int edefCheckAttempt;

string edefName;
assign edefName to "";
string edefNamePV;

string edefUserName;
assign edefUserName to "";
string edefUserNamePV;

string edefNAVG;
assign edefNAVG to "";
string edefNAVGPV;

string edefNRPOS;
assign edefNRPOS to "";
string edefNRPOSPV;

int dgrpINCM[3]; 
assign dgrpINCM to "VX00:DGRP:1150:INCM";
monitor dgrpINCM;

int dgrpEXCM[3];
assign dgrpEXCM to "VX00:DGRP:1150:EXCM";
monitor dgrpEXCM;

unsigned short dgrpSHORT;
short i;

unsigned short INCLUSIONvalSHORT;
assign INCLUSIONvalSHORT to "";
string INCLUSIONpvSHORT;

unsigned short EXCLUSIONvalSHORT;
assign EXCLUSIONvalSHORT to "";
string EXCLUSIONpvSHORT;

entry
{
	edefMaxNumber = 20;

	sprintf (edefRequestName, "");
	pvPut (edefRequestName);

	sprintf (edefNAVG, "1");
	sprintf (edefNRPOS, "1");

        epicsThreadSleep (10.0); /* {} wait for mask setup */

	/* Start all canned edefs */
	for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
          sprintf (edefNamePV, "EDEF:SYS0:%d:NAME", edefNumber);
          pvAssign (edefName, edefNamePV);
          pvGet (edefName);
          if (0 == strlen(edefName)) {
            /* nothing to do */
            }
          else {
            sprintf (edefNamePV, "EDEF:SYS0:%d:CTRL", edefNumber);
            pvAssign (edefName, edefNamePV);
            sprintf (edefName, "ON");
            pvPut (edefName);
            };
	  };

        edefNumber = 0;
        edefReserveAttempt = 0;
        edefCheckAttempt = 0;
}

ss edefReserveStateSet
{
	state waiting 
	{
	    entry
	    {
		efClear(edefRequestName_ef);
	    }
	    when(efTest(edefRequestName_ef))
	    {
		edefCheckAttempt = 0;
	    } state checking 
	}

	state checking
	{	
	    when ((edefCheckAttempt < edefMaxNumber) && (strlen(edefRequestName)>0))
	    {
		edefCheckAttempt++;
                sprintf (edefNamePV, "EDEF:SYS0:%d:NAME", edefCheckAttempt);
                pvAssign (edefName, edefNamePV);
                pvGet (edefName);
		if (0 == strcmp(edefName,edefRequestName))
		{
		  sprintf (edefRequestName, "");
                  pvPut (edefRequestName);
		}
	    } state checking
	    when ((edefCheckAttempt >= edefMaxNumber) && (strlen(edefRequestName)>0))
	    {
		edefReserveAttempt = 0;
	    } state reserving
	    when(1)
	    {
	    } state waiting
	}

	state reserving
	{
	    when(strlen(edefRequestName)>0)
	    {
		edefReserveAttempt++;
		edefNumber++;
		if (edefNumber > edefMaxNumber) edefNumber = 1;
		sprintf (edefNamePV, "EDEF:SYS0:%d:NAME", edefNumber);
		pvAssign (edefName, edefNamePV);
		pvGet (edefName);
		if (0 == strlen(edefName))
		{
		  sprintf (edefName, "%s", edefRequestName);
		  pvPut (edefName);
		  sprintf (edefUserNamePV, "EDEF:SYS0:%d:USERNAME", edefNumber);
		  pvAssign (edefUserName, edefUserNamePV);
		  sprintf (edefUserName, "Client");
		  pvPut (edefUserName);
		  sprintf (edefNAVGPV, "EDEF:SYS0:%d:AVGCNTMAX", edefNumber);
		  pvAssign (edefNAVG, edefNAVGPV);
		  pvPut (edefNAVG);
		  sprintf (edefNRPOSPV, "EDEF:SYS0:%d:MEASCNTMAX", edefNumber);
		  pvAssign (edefNRPOS, edefNRPOSPV);
		  pvPut (edefNRPOS);
                  for (i=0; i<3; i++) {
                    dgrpSHORT = dgrpINCM[i] & 0x00FF;
                    sprintf (INCLUSIONpvSHORT, "EDEF:SYS0:%d:INCLUSION%dA", edefNumber, 1+i);
                    pvAssign (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                    INCLUSIONvalSHORT = dgrpSHORT;
                    pvPut (INCLUSIONvalSHORT);

                    dgrpSHORT = (dgrpINCM[i] & 0xFF00) >> 16;
                    sprintf (INCLUSIONpvSHORT, "EDEF:SYS0:%d:INCLUSION%dB", edefNumber, 1+i);
                    pvAssign (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                    INCLUSIONvalSHORT = dgrpSHORT;
                    pvPut (INCLUSIONvalSHORT);

                    dgrpSHORT = dgrpEXCM[i] & 0x00FF;
                    sprintf (EXCLUSIONpvSHORT, "EDEF:SYS0:%d:EXCLUSION%dA", edefNumber, 1+i);
                    pvAssign (EXCLUSIONvalSHORT, EXCLUSIONpvSHORT);
                    EXCLUSIONvalSHORT = dgrpSHORT;
                    pvPut (EXCLUSIONvalSHORT);

                    dgrpSHORT = (dgrpEXCM[i] & 0xFF00) >> 16;
                    sprintf (EXCLUSIONpvSHORT, "EDEF:SYS0:%d:EXCLUSION%dB", edefNumber, 1+i);
                    pvAssign (EXCLUSIONvalSHORT, EXCLUSIONpvSHORT);
                    EXCLUSIONvalSHORT = dgrpSHORT;
                    pvPut (EXCLUSIONvalSHORT);
                    };
                  sprintf (edefRequestName, "");
                  pvPut (edefRequestName);
		}
		else if (edefReserveAttempt > edefMaxNumber) 
		{
		  sprintf (edefRequestName, "");
		  pvPut (edefRequestName);
		}
	    } state reserving
	    when(1)
	    {
	    } state waiting
	}
}
