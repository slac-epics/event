program edefMask

option -a;  /* All pvGets must be synchronous */
option +c;  /* All connections must be made before execution begins */
option +d;  /* Turn on runtime debug messages */

%%#include <string.h> 	   /* sprintf prototype */

int maskChanged;
assign maskChanged to "IOC:SYS0:1:MASKCHG";
monitor maskChanged;
evflag maskChanged_ef;
sync maskChanged maskChanged_ef;

int dgrpINCM[3];
assign dgrpINCM to "VX00:DGRP:1150:INCM";
monitor dgrpINCM;

int dgrpEXCM[3];
assign dgrpEXCM to "VX00:DGRP:1150:EXCM";
monitor dgrpEXCM;

int initDone;
assign initDone to "IOC:SYS0:1:MASKSETUP";

unsigned short dgrpSHORT;
short i;
short j;
short eachBit;
short bitp;

int edefMaxNumber;
int edefNumber;

int maskMaxNumber;
int maskCount;
int maskNumber;

int pnbnMaxNumber;
int pnbnNumber;

string NAMEval;
assign NAMEval to "";
string NAMEpv;

string PNBNval;
assign PNBNval to "";
string PNBNpv;

int BITPval;
assign BITPval to "";
string BITPpv;

int BITWval;
assign BITWval to "";
string BITWpv;

string DESCval;
assign DESCval to "";
string DESCpv;

string DESC1val;
assign DESC1val to "";
string DESC1pv;

int INCMval;
assign INCMval to "";
string INCMpv;

int EXCMval;
assign EXCMval to "";
string EXCMpv;

string INCMSval;
assign INCMSval to "";
string INCMSpv;
int iINCMS;

string EXCMSval;
assign EXCMSval to "";
string EXCMSpv;
int iEXCMS;

unsigned long INCLUSIONval;
assign INCLUSIONval to "";
string INCLUSIONpv;

unsigned long EXCLUSIONval;
assign EXCLUSIONval to "";
string EXCLUSIONpv;

unsigned short INCLUSIONvalSHORT;
assign INCLUSIONvalSHORT to "";
string INCLUSIONpvSHORT;

unsigned short EXCLUSIONvalSHORT;
assign EXCLUSIONvalSHORT to "";
string EXCLUSIONpvSHORT;

int sort_done;
int debug_flag;

entry {

  debug_flag = 0;
  if (debug_flag) printf ("edefMask:entry\n");

  sort_done = 0;

  edefMaxNumber = 20;
  edefNumber = 0;

  maskMaxNumber = 160;
  maskCount = 0;
  maskNumber = 0;

  pnbnMaxNumber = 124;
  pnbnNumber = 0;
  sprintf (PNBNval, "");

  } /* edefMask entry*/

ss edefMaskStateSet {

  state pnbn_read {

    entry {
      if (debug_flag) printf ("edefMaskStateSet:pnbn_read");

      /* setup first PNBN NAME, BITP & BITW */
      pnbnNumber++;
      sprintf (PNBNpv, "MP00:PNBN:%d:NAME", pnbnNumber);
      pvAssign (PNBNval, PNBNpv);
      pvMonitor (PNBNval);

      BITPval = -1;
      sprintf (BITPpv, "MP00:PNBN:%d:BITP", pnbnNumber);
      pvAssign (BITPval, BITPpv);
      pvMonitor (BITPval);

      BITWval = -1;
      sprintf (BITWpv, "MP00:PNBN:%d:BITW", pnbnNumber);
      pvAssign (BITWval, BITWpv);
      pvMonitor (BITWval);
      } /* pnbn_read entry */

    when (pnbnNumber > pnbnMaxNumber) {
      } state pnbn_sort

    when ((strlen(PNBNval)>0) && (BITPval>-1) && (BITWval>-1)) {
      if ((BITPval < 32) || (0 == strncmp ("            ", PNBNval, 12))) {
        /* unused PNBN name, 32 comes from PNET_MODFBIT_LOLIM */
        }
      else {
        if (debug_flag) printf (".");
        maskCount++;
        /* Put the name in INCM DESC */
        sprintf (DESC1pv, "EDEF:SYS0:1:INCM%d.DESC", maskCount);
        pvAssign (DESC1val, DESC1pv);
        sprintf (DESC1val, "%s", PNBNval);
        pvPut (DESC1val);
        /* Put the BITP & BITW into the EXCM DESC */
        sprintf (DESC1pv, "EDEF:SYS0:1:EXCM%d.DESC", maskCount);
        pvAssign (DESC1val, DESC1pv);
        sprintf (DESC1val, "%3.3d %3.3d", BITPval, BITWval);
        pvPut (DESC1val);
        }; /* non-blank pnbn name */
      pnbnNumber++;
      sprintf (PNBNval, "");
      BITPval = -1;
      BITWval = -1;
      if (pnbnNumber <= pnbnMaxNumber) {
        sprintf (PNBNpv, "MP00:PNBN:%d:NAME", pnbnNumber);
        pvAssign (PNBNval, PNBNpv);
        pvMonitor (PNBNval);
        sprintf (BITPpv, "MP00:PNBN:%d:BITP", pnbnNumber);
        pvAssign (BITPval, BITPpv);
        pvMonitor (BITPval);
        sprintf (BITWpv, "MP00:PNBN:%d:BITW", pnbnNumber);
        pvAssign (BITWval, BITWpv);
        pvMonitor (BITWval);
        }; /* setup next pnbn unit */
      } state pnbn_read
    } /* state pnbn_read */

  state pnbn_sort {
    entry { 
      if (debug_flag) printf ("\nedefMaskStateSet:pnbn_sort"); 
      } 
    when (sort_done) {
      } state pnbn_assign
    when (1) {
      if (debug_flag) printf (".");
      sort_done = 1;
      for (maskNumber = 1; maskNumber < maskCount; maskNumber++) {
        sprintf (DESC1pv, "EDEF:SYS0:1:INCM%d.DESC", maskNumber);
        pvAssign (DESC1val, DESC1pv);
        pvGet (DESC1val);
        sprintf (DESCpv, "EDEF:SYS0:1:INCM%d.DESC", 1+maskNumber);
        pvAssign (DESCval, DESCpv);
        pvGet (DESCval);
        if (strcmp (DESC1val, DESCval) > 0) {
          sort_done = 0;
          /* switch INCM DESC's */
          sprintf (PNBNval, "%s", DESC1val);
          sprintf (DESC1val, "%s", DESCval);
          sprintf (DESCval, "%s", PNBNval);
          pvPut (DESC1val);
          pvPut (DESCval);
          /* switch EXCM DESC's */
          sprintf (DESC1pv, "EDEF:SYS0:1:EXCM%d.DESC", maskNumber);
          pvAssign (DESC1val, DESC1pv);
          pvGet (DESC1val);
          sprintf (DESCpv, "EDEF:SYS0:1:EXCM%d.DESC", 1+maskNumber);
          pvAssign (DESCval, DESCpv);
          pvGet (DESCval);
          sprintf (PNBNval, "%s", DESC1val);
          sprintf (DESC1val, "%s", DESCval);
          sprintf (DESCval, "%s", PNBNval);
          pvPut (DESC1val);
          pvPut (DESCval);
          }; /* switch */
        }; /* for each mask */
      } state pnbn_sort
    } /* state pnbn sort */

  state pnbn_assign {
    entry { 
      if (debug_flag) printf ("\nedefMaskStateSet:pnbn_assign\n"); 
      }
    when (1) {
      for (maskNumber = 1; maskNumber <= maskCount; maskNumber++) {
        sprintf (DESC1pv, "EDEF:SYS0:1:INCM%d.DESC", maskNumber);
        pvAssign (DESC1val, DESC1pv);
        pvGet (DESC1val);
        for (edefNumber = 2; edefNumber <= edefMaxNumber; edefNumber++) {
          sprintf (DESCpv, "EDEF:SYS0:%d:INCM%d.DESC", edefNumber, maskNumber);
          pvAssign (DESCval, DESCpv);
          sprintf (DESCval, "%s", DESC1val);
          pvPut (DESCval);
          }; /* for each edef */
        }; /* for each mask */
      for (maskNumber = 1; maskNumber <= maskCount; maskNumber++) {
        sprintf (DESC1pv, "EDEF:SYS0:1:EXCM%d.DESC", maskNumber);
        pvAssign (DESC1val, DESC1pv);
        pvGet (DESC1val);
        for (edefNumber = 2; edefNumber <= edefMaxNumber; edefNumber++) {
          sprintf (DESCpv, "EDEF:SYS0:%d:EXCM%d.DESC", edefNumber, maskNumber);
          pvAssign (DESCval, DESCpv);
          sprintf (DESCval, "%s", DESC1val);
          pvPut (DESCval);
          }; /* for each edef */
        }; /* for each mask */
      } state dgrp_incm_excm_set
    } /* state pnbn assign */

  state dgrp_incm_excm_set {
    entry {
      if (debug_flag) printf ("edefMaskStateSet:dgrp_incm_excm_set\n"); 
      }
    /* read DGRP INCM & EXCM and set INCLUSION[1..5][A|B] & EXCLUSION[1..5][A|B] */
    when (1) {
      for (i=0; i<3; i++) {
        for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
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
          }; /* for each edef */
        };
        sprintf (INCMSval, "1HZ");
        sprintf (PNBNval, "ONE_HERTZ");
      } state special_incm_excm_set
    } /* state dgrp_incm_excm_set */

  state special_incm_excm_set {
    /* Look for 1Hz and 10Hz eDefs and set their ONE_HERTZ & TEN_HERTZ INCLUSION[1..5][A|B] */
    entry {
      if (debug_flag) printf ("edefMaskStateSet:special_incm_excm_set\n");
      }
    when (0 == strlen(INCMSval)) {
      } state incm_excm_set
    when (1) {
      for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
        sprintf (NAMEpv, "EDEF:SYS0:%d:NAME", edefNumber);
        pvAssign (NAMEval, NAMEpv);
        pvGet (NAMEval);
        if (0 == strncmp (NAMEval, INCMSval, strlen(INCMSval))) {
          for (maskNumber = 1; maskNumber <= maskCount; maskNumber++) {
            sprintf (DESCpv, "EDEF:SYS0:%d:INCM%d.DESC", edefNumber, maskNumber);
            pvAssign (DESCval, DESCpv);
            pvGet (DESCval);
            if (0 == strncmp (DESCval, PNBNval, strlen(PNBNval))) {
              sprintf (DESCpv, "EDEF:SYS0:%d:EXCM%d.DESC", edefNumber, maskNumber);
              pvAssign (DESCval, DESCpv);
              pvGet (DESCval);
              sscanf (DESCval, "%3d %3d", &BITPval, &BITWval);
              for (eachBit = 0; eachBit < BITWval; eachBit++) {
                bitp = BITPval + eachBit;
                j = 0; /* EDEF:SYS0:[edefNumbner]:INCLUSION[j] */
                while (bitp > 31) {
                  j++;
                  bitp = bitp - 32;
                  };
                if (bitp < 16) {
                  sprintf (INCLUSIONpvSHORT, "EDEF:SYS0:%d:INCLUSION%dA", edefNumber, j);
                  pvAssign (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                  pvGet (INCLUSIONvalSHORT);
                  INCLUSIONvalSHORT += (1 << bitp);
                  pvPut (INCLUSIONvalSHORT);
                  }
                else {
                  sprintf (INCLUSIONpvSHORT, "EDEF:SYS0:%d:INCLUSION%dB", edefNumber, j);
                  pvAssign (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                  pvGet (INCLUSIONvalSHORT);
                  bitp = bitp - 16;
                  INCLUSIONvalSHORT += (1 << bitp);
                  pvPut (INCLUSIONvalSHORT);
                  };
                }; /* for each BITW */
              }; /* Found mask name */
            }; /* each mask */
          }; /* found correct EDEF NAME */
        }; /* each edef */
      if (0 == strcmp (INCMSval, "1HZ")) {
        sprintf (INCMSval, "10HZ");
        sprintf (PNBNval, "TEN_HERTZ");
        }
      else {
        sprintf (INCMSval, "");
        initDone = 1;
        pvPut (initDone);
        };
      } state special_incm_excm_set
    } /* state special_incm_excm */

  state incm_excm_set {
    /* Set INCMS[1..4] & EXCMS[1..4] and INCM[1..maskCount] & EXCM[1..maskCount]
       based on INCLUSION[1..5] EXCLUSION[1..5] */
    entry {
      if (debug_flag) printf ("edefMaskStateSet:incm_excm_set\n");
      }
    when(1) {
      for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
        iINCMS=1;
        sprintf (INCMSpv, "EDEF:SYS0:%d:INCMS%d", edefNumber, iINCMS);
        pvAssign (INCMSval, INCMSpv);
        sprintf (INCMSval, "");
        iEXCMS=1;
        sprintf (EXCMSpv, "EDEF:SYS0:%d:EXCMS%d", edefNumber, iEXCMS);
        pvAssign (EXCMSval, EXCMSpv);
        sprintf (EXCMSval, "");
        for (maskNumber = 1; maskNumber <= maskCount; maskNumber++) {
          i = 0; /* index into INCLUSION & EXCLUSION */
          /* Is my BITP set? */
          sprintf (DESCpv, "EDEF:SYS0:%d:EXCM%d.DESC", edefNumber, maskNumber);
          pvAssign (DESCval, DESCpv);
          pvGet (DESCval);
          sscanf (DESCval, "%3d", &BITPval);
          sprintf (DESCpv, "EDEF:SYS0:%d:INCM%d.DESC", edefNumber, maskNumber);
          pvAssign (DESCval, DESCpv);
          pvGet (DESCval);
          sprintf (PNBNval, "%s", DESCval); /* remove trailing blanks */
          for (j=0; j<sizeof(PNBNval); j++) if (' ' == PNBNval[j]) PNBNval[j]=0;
          pvGet (DESCval);
          while (BITPval > 31) {
            i++;
            BITPval = BITPval - 32;
            };
          sprintf (INCLUSIONpv, "EDEF:SYS0:%d:INCLUSION%d", edefNumber, i);
          pvAssign (INCLUSIONval, INCLUSIONpv);
          pvGet (INCLUSIONval);
          sprintf (EXCLUSIONpv, "EDEF:SYS0:%d:EXCLUSION%d", edefNumber, i);
          pvAssign (EXCLUSIONval, EXCLUSIONpv);
          pvGet (EXCLUSIONval);
          sprintf (INCMpv, "EDEF:SYS0:%d:INCM%d", edefNumber, maskNumber);
          pvAssign (INCMval, INCMpv);
          sprintf (EXCMpv, "EDEF:SYS0:%d:EXCM%d", edefNumber, maskNumber);
          pvAssign (EXCMval, EXCMpv);
          if (INCLUSIONval & (1 << BITPval)) {
            INCMval = 1;
            if ((strlen(INCMSval) + strlen(PNBNval)) > 38) {
              pvPut (INCMSval);
              sprintf (INCMSpv, "EDEF:SYS0:%d:INCMS%d", edefNumber, ++iINCMS);
              pvAssign (INCMSval, INCMSpv);
              sprintf (INCMSval, "");
              };
            if (iINCMS < 5) {
              if (0 == strlen(INCMSval)) {
                sprintf (INCMSval, "%s", PNBNval);
                }
              else {
                sprintf (INCMSval, "%s %s", INCMSval, PNBNval); 
                };
              };
            } /* INCLUSION bit set */
          else {
            INCMval = 0;
            }; /* INCLUSION bit not set */
          pvPut (INCMval);
          if (EXCLUSIONval & (1 << BITPval)) {
            EXCMval = 1;
            if ((strlen(EXCMSval) + strlen(PNBNval)) > 38) {
              pvPut (EXCMSval);
              sprintf (EXCMSpv, "EDEF:SYS0:%d:EXCMS%d", edefNumber, ++iEXCMS);
              pvAssign (EXCMSval, EXCMSpv);
              sprintf (EXCMSval, "");
              };
            if (iEXCMS < 5) {
              if (0 == strlen(EXCMSval)) {
                sprintf (EXCMSval, "%s", PNBNval);
                }
              else {
                sprintf (EXCMSval, "%s %s", EXCMSval, PNBNval);
                };
              };
            } /* EXCLUSION bit set */
          else {
            EXCMval = 0;
            }; /* EXCLUSION bit not set */
          pvPut (EXCMval);
          }; /* for each Mask */
        while (iINCMS < 5) {
          pvPut (INCMSval);
          sprintf (INCMSpv, "EDEF:SYS0:%d:INCMS%d", edefNumber, ++iINCMS);
          pvAssign (INCMSval, INCMSpv);
          sprintf (INCMSval, "");
          };
        while (iEXCMS < 5) {
          pvPut (EXCMSval);
          sprintf (EXCMSpv, "EDEF:SYS0:%d:EXCMS%d", edefNumber, ++iEXCMS);
          pvAssign (EXCMSval, EXCMSpv);
          sprintf (EXCMSval, "");
          };
        }; /* for each edef */
      } state checking
    } /* state incm_excm_set */

  state checking { 
    /* Check for changes in INCM[1..maskCount] & EXCM[1..maskCount] and set 
       INCLUSION[1..5][A|B] & EXCLUSION[1..5][A|B] */
    entry {
      if (debug_flag) printf ("edefMaskStateSet:checking\n");
      efClear (maskChanged_ef);
      }
    when (efTest(maskChanged_ef)) {
      for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
        for (maskNumber = 1; maskNumber <= maskCount; maskNumber++) {
          sprintf (INCMpv, "EDEF:SYS0:%d:INCM%d", edefNumber, maskNumber);
          pvAssign (INCMval, INCMpv);
          pvGet (INCMval);
          /* Set INCLUSION[1..5][A|B] */
          sprintf (DESCpv, "EDEF:SYS0:%d:EXCM%d.DESC", edefNumber, maskNumber);
          pvAssign (DESCval, DESCpv);
          pvGet (DESCval);
          sscanf (DESCval, "%3d %3d", &BITPval, &BITWval);
          for (eachBit = 0; eachBit < BITWval; eachBit++) {
            bitp = BITPval + eachBit;
            j = 0; /* EDEF:SYS0:[edefNumbner]:INCLUSION[j] */
            while (bitp > 31) {
              j++;
              bitp = bitp - 32;
              };
            if (bitp < 16) {
              sprintf (INCLUSIONpvSHORT, "EDEF:SYS0:%d:INCLUSION%dA", edefNumber, j);
              pvAssign (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
              pvGet (INCLUSIONvalSHORT);
              if (INCMval) {
                if (! (INCLUSIONvalSHORT & (1 << bitp))) INCLUSIONvalSHORT += (1 << bitp);
                }
              else {
                if (INCLUSIONvalSHORT & (1 << bitp)) INCLUSIONvalSHORT = INCLUSIONvalSHORT - (1 << bitp);
                }
              pvPut (INCLUSIONvalSHORT);
              }
            else {
              sprintf (INCLUSIONpvSHORT, "EDEF:SYS0:%d:INCLUSION%dB", edefNumber, j);
              pvAssign (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
              pvGet (INCLUSIONvalSHORT);
              bitp = bitp - 16;
              if (INCMval) {
                if (! (INCLUSIONvalSHORT & (1 << bitp))) INCLUSIONvalSHORT += (1 << bitp);
                }
              else {
                if (INCLUSIONvalSHORT & (1 << bitp)) INCLUSIONvalSHORT = INCLUSIONvalSHORT - (1 << bitp);
                };
              pvPut (INCLUSIONvalSHORT);
              };
            }; /* for each BITW */

          sprintf (EXCMpv, "EDEF:SYS0:%d:EXCM%d", edefNumber, maskNumber);
          pvAssign (EXCMval, EXCMpv);
          pvGet (EXCMval);
          /* Set EXCLUSION[1..5][A|B] */
          sprintf (DESCpv, "EDEF:SYS0:%d:EXCM%d.DESC", edefNumber, maskNumber);
          pvAssign (DESCval, DESCpv);
          pvGet (DESCval);
          sscanf (DESCval, "%3d %3d", &BITPval, &BITWval);
          for (eachBit = 0; eachBit < BITWval; eachBit++) {
            bitp = BITPval + eachBit;
            j = 0; /* EDEF:SYS0:[edefNumbner]:EXCLUSION[j] */
            while (bitp > 31) {
              j++;
              bitp = bitp - 32;
              };
            if (bitp < 16) {
              sprintf (EXCLUSIONpvSHORT, "EDEF:SYS0:%d:EXCLUSION%dA", edefNumber, j);
              pvAssign (EXCLUSIONvalSHORT, EXCLUSIONpvSHORT);
              pvGet (EXCLUSIONvalSHORT);
              if (EXCMval) {
                if (! (EXCLUSIONvalSHORT & (1 << bitp))) EXCLUSIONvalSHORT += (1 << bitp);
                }
              else {
                if (EXCLUSIONvalSHORT & (1 << bitp)) EXCLUSIONvalSHORT = EXCLUSIONvalSHORT - (1 << bitp);
                }
              pvPut (EXCLUSIONvalSHORT);
              }
            else {
              sprintf (EXCLUSIONpvSHORT, "EDEF:SYS0:%d:EXCLUSION%dB", edefNumber, j);
              pvAssign (EXCLUSIONvalSHORT, EXCLUSIONpvSHORT);
              pvGet (EXCLUSIONvalSHORT);
              bitp = bitp - 16;
              if (EXCMval) {
                if (! (EXCLUSIONvalSHORT & (1 << bitp))) EXCLUSIONvalSHORT += (1 << bitp);
                }
              else {
                if (EXCLUSIONvalSHORT & (1 << bitp)) EXCLUSIONvalSHORT = EXCLUSIONvalSHORT - (1 << bitp);
                };
              pvPut (EXCLUSIONvalSHORT);
              };
            }; /* for each BITW */
          }; /* each mask */
        }; /* each edef */
      } state incm_excm_set
    } /* state checking */

} /* ss edefMaskStateSet */
