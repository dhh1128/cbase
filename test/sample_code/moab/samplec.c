/* HEADER */

#include <stdio.h>
#include "cmoab.h"


/**
 *
 *
 */

int main()

  {
  char *ptr;

  char *CmdString = "0 ALL 0";

  MCInitialize(&C,&MSched,NULL,0,TRUE,TRUE);

  MCDoCommand(
    mcsShowQueue,
    &C,
    CmdString,
    &ptr,
    NULL,
    NULL);

  fprintf(stdout,"R: '%s'\n",
    ptr);

  exit(0);
  }
