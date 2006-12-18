program edefMask

%%#include <string.h> 	   /* sprintf prototype            */

#include "pvMacros.st"

string edefRequestName;
assign edefRequestName to "EVG:SYS0:1:EDEFNAME";
monitor edefRequestName;
string prevedefRequestName;

int dgrpINCM[3];
assign dgrpINCM to "VX00:DGRP:1150:INCM";
monitor dgrpINCM;

int dgrpEXCM[3];
assign dgrpEXCM to "VX00:DGRP:1150:EXCM";
monitor dgrpEXCM;

unsigned short dgrpSHORT;
short i;
short j;
short eachBit;
short bitp;

int edefMaxNumber;
assign edefMaxNumber to "EVG:SYS0:1:EDEFMAXCOUNT";
int edefNumber;

int maskMaxNumber;
assign maskMaxNumber to "EVG:SYS0:1:MASKMAXCOUNT";
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

int *data_p;
int somethingChanged;
int sort_done;
int debug_flag;

entry {

  delay_time = 0.01;
  debug_flag = 0;

  sprintf (prevedefRequestName, "");

  data_p = 0;
  somethingChanged = 0;
  sort_done = 0;

  PVGET (edefMaxNumber);
  edefNumber = 0;

  PVGET (maskMaxNumber);
  maskCount = 0;
  maskNumber = 0;

  pnbnMaxNumber = 124;
  pnbnNumber = 0;
  sprintf (PNBNval, "");

  /* setup first PNBN NAME, BITP & BITW */
  pnbnNumber++;
  sprintf (PNBNpv, "MP00:PNBN:%d:NAME", pnbnNumber);
  PVASSIGN (PNBNval, PNBNpv);
  pvMonitor (PNBNval);

  BITPval = -1;
  sprintf (BITPpv, "MP00:PNBN:%d:BITP", pnbnNumber);
  PVASSIGN (BITPval, BITPpv);
  pvMonitor (BITPval);

  BITWval = -1;
  sprintf (BITWpv, "MP00:PNBN:%d:BITW", pnbnNumber);
  PVASSIGN (BITWval, BITWpv);
  pvMonitor (BITWval);
  }

ss edefMaskStateSet {

  state pnbn_read {
    entry { 
      if (debug_flag) printf ("pnbn_read\n"); 
      }
    when (pnbnNumber > pnbnMaxNumber) {
      } state pnbn_sort
    when (delay (delay_time)) {
      if (0 == strlen(PNBNval) || (-1 == BITPval) || (-1 == BITWval)) {
        /* Didn't connect to PNBNpv yet, delay a bit longer */
        }
      else {
        if ((BITPval < 32) || (0 == strncmp ("            ", PNBNval, 12))) {
          /* unused PNBN name, 32 comes from PNET_MODFBIT_LOLIM */
          }
        else {
          maskCount++;
          /* Put the name in INCM DESC */
          sprintf (DESC1pv, "EDEF:SYS0:1:INCM%d.DESC", maskCount);
          PVASSIGN (DESC1val, DESC1pv);
          sprintf (DESC1val, "%s", PNBNval);
          PVPUT (DESC1val);
          /* Put the BITP & BITW into the EXCM DESC */
          sprintf (DESC1pv, "EDEF:SYS0:1:EXCM%d.DESC", maskCount);
          PVASSIGN (DESC1val, DESC1pv);
          sprintf (DESC1val, "%3.3d %3.3d", BITPval, BITWval);
          PVPUT (DESC1val);
          }; /* non-blank pnbn name */
        pnbnNumber++;
        sprintf (PNBNval, "");
        BITPval = -1;
        BITWval = -1;
        if (pnbnNumber <= pnbnMaxNumber) {
          sprintf (PNBNpv, "MP00:PNBN:%d:NAME", pnbnNumber);
          PVASSIGN (PNBNval, PNBNpv);
          pvMonitor (PNBNval);
          sprintf (BITPpv, "MP00:PNBN:%d:BITP", pnbnNumber);   
          PVASSIGN (BITPval, BITPpv);   
          pvMonitor (BITPval);
          sprintf (BITWpv, "MP00:PNBN:%d:BITW", pnbnNumber);
          PVASSIGN (BITWval, BITWpv);
          pvMonitor (BITWval);
          }; /* setup next pnbn unit */
        }; /* didn't get PNBN val, try again. */
      } state pnbn_read
    } /* state pnbn_read */

  state pnbn_sort {
    entry { 
      if (debug_flag) printf ("pnbn_sort\n"); 
      } 
    when (sort_done) {
      } state pnbn_assign
    when (delay(delay_time)) {
      if (debug_flag) printf (".");
      sort_done = 1;
      for (maskNumber = 1; maskNumber < maskCount; maskNumber++) {
        sprintf (DESC1pv, "EDEF:SYS0:1:INCM%d.DESC", maskNumber);
        PVASSIGN (DESC1val, DESC1pv);
        PVGET (DESC1val);
        sprintf (DESCpv, "EDEF:SYS0:1:INCM%d.DESC", 1+maskNumber);
        PVASSIGN (DESCval, DESCpv);
        PVGET (DESCval);
        if (strcmp (DESC1val, DESCval) > 0) {
          sort_done = 0;
          /* switch INCM DESC's */
          sprintf (PNBNval, "%s", DESC1val);
          sprintf (DESC1val, "%s", DESCval);
          sprintf (DESCval, "%s", PNBNval);
          PVPUT (DESC1val);
          PVPUT (DESCval);
          /* switch EXCM DESC's */
          sprintf (DESC1pv, "EDEF:SYS0:1:EXCM%d.DESC", maskNumber);
          PVASSIGN (DESC1val, DESC1pv);
          PVGET (DESC1val);
          sprintf (DESCpv, "EDEF:SYS0:1:EXCM%d.DESC", 1+maskNumber);
          PVASSIGN (DESCval, DESCpv);
          PVGET (DESCval);
          sprintf (PNBNval, "%s", DESC1val);
          sprintf (DESC1val, "%s", DESCval);
          sprintf (DESCval, "%s", PNBNval);
          PVPUT (DESC1val);
          PVPUT (DESCval);
          }; /* switch */
        }; /* for each mask */
      } state pnbn_sort
    } /* state pnbn sort */

  state pnbn_assign {
    entry { 
      if (debug_flag) printf ("\npnbn_assign\n"); 
      }
    when (delay(delay_time)) {
      for (maskNumber = 1; maskNumber <= maskCount; maskNumber++) {
        sprintf (DESC1pv, "EDEF:SYS0:1:INCM%d.DESC", maskNumber);
        PVASSIGN (DESC1val, DESC1pv);
        PVGET (DESC1val);
        for (edefNumber = 2; edefNumber <= edefMaxNumber; edefNumber++) {
          sprintf (DESCpv, "EDEF:SYS0:%d:INCM%d.DESC", edefNumber, maskNumber);
          PVASSIGN (DESCval, DESCpv);
          sprintf (DESCval, "%s", DESC1val);
          PVPUT (DESCval);
          }; /* for each edef */
        }; /* for each mask */
      for (maskNumber = 1; maskNumber <= maskCount; maskNumber++) {
        sprintf (DESC1pv, "EDEF:SYS0:1:EXCM%d.DESC", maskNumber);
        PVASSIGN (DESC1val, DESC1pv);
        PVGET (DESC1val);
        for (edefNumber = 2; edefNumber <= edefMaxNumber; edefNumber++) {
          sprintf (DESCpv, "EDEF:SYS0:%d:EXCM%d.DESC", edefNumber, maskNumber);
          PVASSIGN (DESCval, DESCpv);
          sprintf (DESCval, "%s", DESC1val);
          PVPUT (DESCval);
          }; /* for each edef */
        }; /* for each mask */
      } state dgrp_incm_excm_set
    } /* state pnbn assign */

  state dgrp_incm_excm_set {
    entry {
      if (debug_flag) printf ("dgrp_incm_excm_set\n"); 
      }
    /* read DGRP INCM & EXCM and set INCLUSION[1..5][A|B] & EXCLUSION[1..5][A|B] */
    when (delay(delay_time)) {
      for (i=0; i<3; i++) {
        for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
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
          }; /* for each edef */
        };
        sprintf (INCMSval, "1HZ");
        sprintf (PNBNval, "ONE_HERTZ");
      } state special_incm_excm_set
    } /* state dgrp_incm_excm */

  state special_incm_excm_set {
    /* Look for 1Hz and 10Hz eDefs and set their ONE_HERTZ & TEN_HERTZ INCLUSION[1..5][A|B] */
    entry {
      if (debug_flag) printf ("special_incm_excm_set\n");
      }
    when (0 == strlen(INCMSval)) {
      } state incm_excm_set
    when (delay(delay_time)) {
      for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
        sprintf (NAMEpv, "EDEF:SYS0:%d:NAME", edefNumber);
        PVASSIGN (NAMEval, NAMEpv);
        PVGET (NAMEval);
        if (0 == strncmp (NAMEval, INCMSval, strlen(INCMSval))) {
          for (maskNumber = 1; maskNumber <= maskCount; maskNumber++) {
            sprintf (DESCpv, "EDEF:SYS0:%d:INCM%d.DESC", edefNumber, maskNumber);
            PVASSIGN (DESCval, DESCpv);
            PVGET (DESCval);
            if (0 == strncmp (DESCval, PNBNval, strlen(PNBNval))) {
              sprintf (DESCpv, "EDEF:SYS0:%d:EXCM%d.DESC", edefNumber, maskNumber);
              PVASSIGN (DESCval, DESCpv);
              PVGET (DESCval);
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
                  PVASSIGN (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                  PVGET (INCLUSIONvalSHORT);
                  INCLUSIONvalSHORT += (1 << bitp);
                  PVPUT (INCLUSIONvalSHORT);
                  }
                else {
                  sprintf (INCLUSIONpvSHORT, "EDEF:SYS0:%d:INCLUSION%dB", edefNumber, j);
                  PVASSIGN (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                  PVGET (INCLUSIONvalSHORT);
                  bitp = bitp - 16;
                  INCLUSIONvalSHORT += (1 << bitp);
                  PVPUT (INCLUSIONvalSHORT);
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
        };
      } state special_incm_excm_set
    } /* state special_incm_excm */

  state incm_excm_set {
    /* Set INCMS[1..4] & EXCMS[1..4] and INCM[1..maskCount] & EXCM[1..maskCount]
       based on INCLUSION[1..5] EXCLUSION[1..5] */
    entry {
      if (debug_flag) printf ("incm_excm_set\n");
      }
    when(delay(delay_time)) {
      for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
        iINCMS=1;
        sprintf (INCMSpv, "EDEF:SYS0:%d:INCMS%d", edefNumber, iINCMS);
        PVASSIGN (INCMSval, INCMSpv);
        sprintf (INCMSval, "");
        iEXCMS=1;
        sprintf (EXCMSpv, "EDEF:SYS0:%d:EXCMS%d", edefNumber, iEXCMS);
        PVASSIGN (EXCMSval, EXCMSpv);
        sprintf (EXCMSval, "");
        for (maskNumber = 1; maskNumber <= maskCount; maskNumber++) {
          i = 0; /* index into INCLUSION & EXCLUSION */
          /* Is my BITP set? */
          sprintf (DESCpv, "EDEF:SYS0:%d:EXCM%d.DESC", edefNumber, maskNumber);
          PVASSIGN (DESCval, DESCpv);
          PVGET (DESCval);
          sscanf (DESCval, "%3d", &BITPval);
          sprintf (DESCpv, "EDEF:SYS0:%d:INCM%d.DESC", edefNumber, maskNumber);
          PVASSIGN (DESCval, DESCpv);
          PVGET (DESCval);
          sprintf (PNBNval, "%s", DESCval); /* remove trailing blanks */
          for (j=0; j<sizeof(PNBNval); j++) if (' ' == PNBNval[j]) PNBNval[j]=0;
          PVGET (DESCval);
          while (BITPval > 31) {
            i++;
            BITPval = BITPval - 32;
            };
          sprintf (INCLUSIONpv, "EDEF:SYS0:%d:INCLUSION%d", edefNumber, i);
          PVASSIGN (INCLUSIONval, INCLUSIONpv);
          PVGET (INCLUSIONval);
          sprintf (EXCLUSIONpv, "EDEF:SYS0:%d:EXCLUSION%d", edefNumber, i);
          PVASSIGN (EXCLUSIONval, EXCLUSIONpv);
          PVGET (EXCLUSIONval);
          sprintf (INCMpv, "EDEF:SYS0:%d:INCM%d", edefNumber, maskNumber);
          PVASSIGN (INCMval, INCMpv);
          sprintf (EXCMpv, "EDEF:SYS0:%d:EXCM%d", edefNumber, maskNumber);
          PVASSIGN (EXCMval, EXCMpv);
          if (INCLUSIONval & (1 << BITPval)) {
            INCMval = 1;
            if ((strlen(INCMSval) + strlen(PNBNval)) > 38) {
              PVPUT (INCMSval);
              sprintf (INCMSpv, "EDEF:SYS0:%d:INCMS%d", edefNumber, ++iINCMS);
              PVASSIGN (INCMSval, INCMSpv);
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
          PVPUT (INCMval);
          if (EXCLUSIONval & (1 << BITPval)) {
            EXCMval = 1;
            if ((strlen(EXCMSval) + strlen(PNBNval)) > 38) {
              PVPUT (EXCMSval);
              sprintf (EXCMSpv, "EDEF:SYS0:%d:EXCMS%d", edefNumber, ++iEXCMS);
              PVASSIGN (EXCMSval, EXCMSpv);
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
          PVPUT (EXCMval);
          }; /* for each Mask */
        while (iINCMS < 5) {
          PVPUT (INCMSval);
          sprintf (INCMSpv, "EDEF:SYS0:%d:INCMS%d", edefNumber, ++iINCMS);
          PVASSIGN (INCMSval, INCMSpv);
          sprintf (INCMSval, "");
          };
        while (iEXCMS < 5) {
          PVPUT (EXCMSval);
          sprintf (EXCMSpv, "EDEF:SYS0:%d:EXCMS%d", edefNumber, ++iEXCMS);
          PVASSIGN (EXCMSval, EXCMSpv);
          sprintf (EXCMSval, "");
          };
        }; /* for each edef */
      } state incm_excm_save 
    } /* state incm_excm_set */

  state incm_excm_save {
    /* Save state of INCM[1..maskCount] & EXCM[1..maskCount] for later checking */
    entry {
      if (debug_flag) printf ("incm_excm_save\n");
      }
    when(delay(delay_time)) {
      if (!data_p) {
        data_p = calloc (2 * edefMaxNumber * maskCount, sizeof(INCMval));
        };
      if (data_p) {
        i = -1;
        for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
          for (maskNumber = 1; maskNumber <= maskCount; maskNumber++) {
            sprintf (INCMpv, "EDEF:SYS0:%d:INCM%d", edefNumber, maskNumber);
            PVASSIGN (INCMval, INCMpv);
            PVGET (INCMval);
            data_p[i++] = INCMval;
            sprintf (EXCMpv, "EDEF:SYS0:%d:EXCM%d", edefNumber, maskNumber);
            PVASSIGN (EXCMval, EXCMpv);
            PVGET (EXCMval);
            data_p[i++] = EXCMval;
            }; /* each mask */
          }; /* each edef */
        }; /* valid data_p */
      } state checking
    } /* state incm_excm_save */

  state checking { 
    /* Check for changes in INCM[1..maskCount] & EXCM[1..maskCount] and set 
       INCLUSION[1..5][A|B] & EXCLUSION[1..5][A|B] */
    entry {
      if (debug_flag) printf ("checking\n");
      }
    when (somethingChanged) {
      somethingChanged = 0;
      } state incm_excm_set
    when (! (0 == strcmp (edefRequestName, prevedefRequestName))) {
      sprintf (prevedefRequestName, "%s", edefRequestName);
      } state incm_excm_set
    when(delay(1)) {
      if (data_p) {
        i = -1;
        for (edefNumber = 1; edefNumber <= edefMaxNumber; edefNumber++) {
          for (maskNumber = 1; maskNumber <= maskCount; maskNumber++) {
            sprintf (INCMpv, "EDEF:SYS0:%d:INCM%d", edefNumber, maskNumber);
            PVASSIGN (INCMval, INCMpv);
            PVGET (INCMval);
            if (data_p[i++] == INCMval) {
              /* nothing changed */
              }
            else { /* Set INCLUSION[1..5][A|B] */
              somethingChanged = 1;
              sprintf (DESCpv, "EDEF:SYS0:%d:EXCM%d.DESC", edefNumber, maskNumber);
              PVASSIGN (DESCval, DESCpv);
              PVGET (DESCval);
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
                  PVASSIGN (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                  PVGET (INCLUSIONvalSHORT);
                  if (INCMval) {
                    if (! (INCLUSIONvalSHORT & (1 << bitp))) INCLUSIONvalSHORT += (1 << bitp);
                    }
                  else {
                    if (INCLUSIONvalSHORT & (1 << bitp)) INCLUSIONvalSHORT = INCLUSIONvalSHORT - (1 << bitp);
                    }
                  PVPUT (INCLUSIONvalSHORT);
                  }
                else {
                  sprintf (INCLUSIONpvSHORT, "EDEF:SYS0:%d:INCLUSION%dB", edefNumber, j);
                  PVASSIGN (INCLUSIONvalSHORT, INCLUSIONpvSHORT);
                  PVGET (INCLUSIONvalSHORT);
                  bitp = bitp - 16;
                  if (INCMval) {
                    if (! (INCLUSIONvalSHORT & (1 << bitp))) INCLUSIONvalSHORT += (1 << bitp);
                    }
                  else {
                    if (INCLUSIONvalSHORT & (1 << bitp)) INCLUSIONvalSHORT = INCLUSIONvalSHORT - (1 << bitp);
                    };
                  PVPUT (INCLUSIONvalSHORT);
                  };
                }; /* for each BITW */
              }; /* Set INCLUSION[1..5][A|B] */
            sprintf (EXCMpv, "EDEF:SYS0:%d:EXCM%d", edefNumber, maskNumber);
            PVASSIGN (EXCMval, EXCMpv);
            PVGET (EXCMval);
            if (data_p[i++] == EXCMval) {
              /* nothing changed */
              }
            else { /* Set EXCLUSION[1..5][A|B] */
              somethingChanged = 1;
              sprintf (DESCpv, "EDEF:SYS0:%d:EXCM%d.DESC", edefNumber, maskNumber);
              PVASSIGN (DESCval, DESCpv);
              PVGET (DESCval);
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
                  PVASSIGN (EXCLUSIONvalSHORT, EXCLUSIONpvSHORT);
                  PVGET (EXCLUSIONvalSHORT);
                  if (EXCMval) {
                    if (! (EXCLUSIONvalSHORT & (1 << bitp))) EXCLUSIONvalSHORT += (1 << bitp);
                    }
                  else {
                    if (EXCLUSIONvalSHORT & (1 << bitp)) EXCLUSIONvalSHORT = EXCLUSIONvalSHORT - (1 << bitp);
                    }
                  PVPUT (EXCLUSIONvalSHORT);
                  }
                else {
                  sprintf (EXCLUSIONpvSHORT, "EDEF:SYS0:%d:EXCLUSION%dB", edefNumber, j);
                  PVASSIGN (EXCLUSIONvalSHORT, EXCLUSIONpvSHORT);
                  PVGET (EXCLUSIONvalSHORT);
                  bitp = bitp - 16;
                  if (EXCMval) {
                    if (! (EXCLUSIONvalSHORT & (1 << bitp))) EXCLUSIONvalSHORT += (1 << bitp);
                    }
                  else {
                    if (EXCLUSIONvalSHORT & (1 << bitp)) EXCLUSIONvalSHORT = EXCLUSIONvalSHORT - (1 << bitp);
                    };
                  PVPUT (EXCLUSIONvalSHORT);
                  };
                }; /* for each BITW */
              }; /* Set EXCLUSION[1..5][A|B] */
            }; /* each mask */
          }; /* each edef */
        }; /* valid data_p */
      } state checking 
    } /* state checking */

} /* ss edefMaskStateSet */
