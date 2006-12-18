program edefReserve

%%#include <stdio.h>

#include "pvMacros.st"

string edefRequestName;
assign edefRequestName to "EVG:SYS0:1:EDEFNAME";
monitor edefRequestName;

int edefMaxNumber;
assign edefMaxNumber to "EVG:SYS0:1:EDEFMAXCOUNT";

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
	delay_time = 0.03;

	sprintf (edefRequestName, "");
	PVPUT (edefRequestName);

	sprintf (edefNAVG, "1");
	sprintf (edefNRPOS, "1");

	PVGET (edefMaxNumber);

        epicsThreadSleep (10.0); /* wait for mask setup */

	/* Start all canned edefs */
	for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
          sprintf (edefNamePV, "EDEF:SYS0:%d:NAME", edefNumber);
          PVASSIGN (edefName, edefNamePV);
          PVGET (edefName);
          if (0 == strlen(edefName)) {
            /* nothing to do */
            }
          else {
            sprintf (edefNamePV, "EDEF:SYS0:%d:CTRL", edefNumber);
            PVASSIGN (edefName, edefNamePV);
            sprintf (edefName, "ON");
            PVPUT (edefName);
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
	    when(strlen(edefRequestName)>0)
	    {
		edefCheckAttempt = 0;
	    } state checking 
	    when(delay(1.0))
	    {
	    } state waiting
	}
	state checking
	{	
	    when ((edefCheckAttempt < edefMaxNumber) && (strlen(edefRequestName)>0))
	    {
		edefCheckAttempt++;
                sprintf (edefNamePV, "EDEF:SYS0:%d:NAME", edefCheckAttempt);
                PVASSIGN (edefName, edefNamePV);
                PVGET (edefName);
		if (0 == strcmp(edefName,edefRequestName))
		{
		  sprintf (edefRequestName, "");
                  PVPUT (edefRequestName);
		}
	    } state checking
	    when ((edefCheckAttempt >= edefMaxNumber) && (strlen(edefRequestName)>0))
	    {
		edefReserveAttempt = 0;
	    } state reserving
	    when(delay(delay_time))
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
		PVASSIGN (edefName, edefNamePV);
		PVGET (edefName);
		if (0 == strlen(edefName))
		{
		  sprintf (edefName, "%s", edefRequestName);
		  PVPUT (edefName);
		  sprintf (edefUserNamePV, "EDEF:SYS0:%d:USERNAME", edefNumber);
		  PVASSIGN (edefUserName, edefUserNamePV);
		  sprintf (edefUserName, "Client");
		  PVPUT (edefUserName);
		  sprintf (edefNAVGPV, "EDEF:SYS0:%d:AVGCNTMAX", edefNumber);
		  PVASSIGN (edefNAVG, edefNAVGPV);
		  PVPUT (edefNAVG);
		  sprintf (edefNRPOSPV, "EDEF:SYS0:%d:MEASCNTMAX", edefNumber);
		  PVASSIGN (edefNRPOS, edefNRPOSPV);
		  PVPUT (edefNRPOS);
                  for (i=0; i<3; i++) {
                    dgrpSHORT = dgrpINCM[i] & 0x00FF;
                    sprintf (INCLUSIONpvSHORT, "EDEF:SYS0:%d:INCLUSION%dA", edefNumber, 1+i);
                    PVASSIGN (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                    INCLUSIONvalSHORT = dgrpSHORT;
                    PVPUT (INCLUSIONvalSHORT);

                    dgrpSHORT = (dgrpINCM[i] & 0xFF00) >> 16;
                    sprintf (INCLUSIONpvSHORT, "EDEF:SYS0:%d:INCLUSION%dB", edefNumber, 1+i);
                    PVASSIGN (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                    INCLUSIONvalSHORT = dgrpSHORT;
                    PVPUT (INCLUSIONvalSHORT);

                    dgrpSHORT = dgrpEXCM[i] & 0x00FF;
                    sprintf (EXCLUSIONpvSHORT, "EDEF:SYS0:%d:EXCLUSION%dA", edefNumber, 1+i);
                    PVASSIGN (EXCLUSIONvalSHORT, EXCLUSIONpvSHORT);
                    EXCLUSIONvalSHORT = dgrpSHORT;
                    PVPUT (EXCLUSIONvalSHORT);

                    dgrpSHORT = (dgrpEXCM[i] & 0xFF00) >> 16;
                    sprintf (EXCLUSIONpvSHORT, "EDEF:SYS0:%d:EXCLUSION%dB", edefNumber, 1+i);
                    PVASSIGN (EXCLUSIONvalSHORT, EXCLUSIONpvSHORT);
                    EXCLUSIONvalSHORT = dgrpSHORT;
                    PVPUT (EXCLUSIONvalSHORT);
                    };
                  sprintf (edefRequestName, "");
                  PVPUT (edefRequestName);
		}
		else if (edefReserveAttempt > edefMaxNumber) 
		{
		  sprintf (edefRequestName, "");
		  PVPUT (edefRequestName);
		}
	    } state reserving
	    when(delay(delay_time))
	    {
	    } state waiting
	}
}
