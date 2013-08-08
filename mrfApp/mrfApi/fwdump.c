/* This program prints out information from the FPGA bit file.
   Usage e.g.

   fwdump < /dev/era1
   fwdump < bitfile.bit
*/

#include <stdio.h>

main()
{
  char c;
  int params, count;

  for (params = 0; params < 7;)
    {
      c = fgetc(stdin);
      if (!c)
        {
          params++;
          if (params >= 4)
          {
            for (count = fgetc(stdin); count; count--)
              {
                c = fgetc(stdin);
                if (c)
                  putchar(c);
                else
                  putchar('\n');
              }
          }
       }
    }
}

