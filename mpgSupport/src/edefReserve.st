program edefReserve

option -a;  /* All pvGets must be synchronous */
option +c;  /* All connections must be made before execution begins */
option +d;  /* Turn on runtime debug messages */

%%#include <stdio.h>

string  edefRequestName;
assign  edefRequestName to "{IOC}:EDEFNAME";
monitor edefRequestName;

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

unsigned long dgrpINCM[3]; 
assign dgrpINCM to "VX00:DGRP:1150:INCM";
monitor dgrpINCM;

unsigned long dgrpEXCM[3];
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

int MaskSetupDone;
assign MaskSetupDone to "{IOC}:MASKSETUP";
monitor MaskSetupDone;

char * SYS;

entry
{
	edefMaxNumber = 20;

	sprintf (edefRequestName, "");
	pvPut (edefRequestName);

	sprintf (edefNAVG, "1");
	sprintf (edefNRPOS, "1");

        edefNumber = 0;
        edefReserveAttempt = 0;
        edefCheckAttempt = 0;

	SYS = macValueGet("SYS");
}

ss edefReserveStateSet
{
	state waitforMaskSetup
	{	
	    when (1 == MaskSetupDone)
	    {
		/* Start all canned edefs */
		for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
		  sprintf (edefNamePV, "EDEF:%s:%d:NAME", SYS, edefNumber);
		  pvAssign (edefName, edefNamePV);
		  pvGet (edefName);
		  if (0 == strlen(edefName)) {
		    /* nothing to do */
		    }
		  else {
		    sprintf (edefNamePV, "EDEF:%s:%d:CTRL", SYS, edefNumber);
		    pvAssign (edefName, edefNamePV);
		    sprintf (edefName, "ON");
		    pvPut (edefName);
		  };
		};
	    } state waiting
	} /* state waitforMaskSetup */

	state waiting 
	{
	    when(strlen(edefRequestName)>0)
	    {
		edefCheckAttempt = 0;
	    } state checking 
	}

	state checking
	{	
	    when ((edefCheckAttempt < edefMaxNumber) && (strlen(edefRequestName)>0))
	    {
		edefCheckAttempt++;
                sprintf (edefNamePV, "EDEF:%s:%d:NAME", SYS, edefCheckAttempt);
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
		sprintf (edefNamePV, "EDEF:%s:%d:NAME", SYS, edefNumber);
		pvAssign (edefName, edefNamePV);
		pvGet (edefName);
		if (0 == strlen(edefName))
		{
		  sprintf (edefName, "%s", edefRequestName);
		  pvPut (edefName);
		  sprintf (edefUserNamePV, "EDEF:%s:%d:USERNAME", SYS, edefNumber);
		  pvAssign (edefUserName, edefUserNamePV);
		  sprintf (edefUserName, "Client");
		  pvPut (edefUserName);
		  sprintf (edefNAVGPV, "EDEF:%s:%d:AVGCNTMAX", SYS, edefNumber);
		  pvAssign (edefNAVG, edefNAVGPV);
		  pvPut (edefNAVG);
		  sprintf (edefNRPOSPV, "EDEF:%s:%d:MEASCNTMAX", SYS, edefNumber);
		  pvAssign (edefNRPOS, edefNRPOSPV);
		  pvPut (edefNRPOS);
                  for (i=0; i<3; i++) {
                    dgrpSHORT = dgrpINCM[i] & 0x0000FFFF;
                    sprintf (INCLUSIONpvSHORT, "EDEF:%s:%d:INCLUSION%dA", SYS, edefNumber, 1+i);
                    pvAssign (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                    INCLUSIONvalSHORT = dgrpSHORT;
                    pvPut (INCLUSIONvalSHORT);

                    dgrpSHORT = (dgrpINCM[i] & 0xFFFF0000) >> 16;
                    sprintf (INCLUSIONpvSHORT, "EDEF:%s:%d:INCLUSION%dB", SYS, edefNumber, 1+i);
                    pvAssign (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                    INCLUSIONvalSHORT = dgrpSHORT;
                    pvPut (INCLUSIONvalSHORT);

                    dgrpSHORT = dgrpEXCM[i] & 0x0000FFFF;
                    sprintf (EXCLUSIONpvSHORT, "EDEF:%s:%d:EXCLUSION%dA", SYS, edefNumber, 1+i);
                    pvAssign (EXCLUSIONvalSHORT, EXCLUSIONpvSHORT);
                    EXCLUSIONvalSHORT = dgrpSHORT;
                    pvPut (EXCLUSIONvalSHORT);

                    dgrpSHORT = (dgrpEXCM[i] & 0xFFFF0000) >> 16;
                    sprintf (EXCLUSIONpvSHORT, "EDEF:%s:%d:EXCLUSION%dB", SYS, edefNumber, 1+i);
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
