#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <endian.h>
#include <byteswap.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEVICE "/dev/mrfevr3"

#define MEM_WINDOW 0x00010000

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define be16_to_cpu(x) bswap_16(x)
#define be32_to_cpu(x) bswap_32(x)
#else
#define be16_to_cpu(x) ((unsigned short)(x))
#define be32_to_cpu(x) ((unsigned long)(x))
#endif

int main(int argc, char *argv[])
{
  char s[20];
  int fdEvr;
  void *pa;
  int i, j;
  int *pEvr;
  int p = 0, d;
  int chmode;
  int dmode = 4; /* access mode: 4 - long word,
		    2 - short, 1 - byte */
  int mmode = 4; /* access mode: 4 - long word,
		    2 - short, 1 - byte */
  char *sm;


  if (argc < 2)
    {
      printf("Usage: %s /dev/mrfevra3\n", argv[0]);
      return -1;
    }

  fdEvr = open(argv[1], O_RDWR);
  if (!fdEvr)
    {
      printf("Could not open %s\n", argv[1]);
      return -1;
    }
  if (fdEvr)
    {
      pa = mmap(0, MEM_WINDOW, PROT_READ | PROT_WRITE, MAP_SHARED, fdEvr, 0);
      if (pa == MAP_FAILED)
        {
          printf("mmap failed.\n");
        }
      if (pa != MAP_FAILED)
	{
	  printf("Buffer %08lx\n", (unsigned long) pa);
	  pEvr = (int *) pa;
	  do
	    {
	      printf("test-> ");
	      fflush(stdout);
	      fgets(s, sizeof(s), stdin);

	      if (s[1] == ' ' || isdigit(s[1]) || isdigit(s[2]))
		p = strtoul(&s[1], NULL, 0);
	      
	      chmode = 0;
	      sm = strchr(s, ',');
	      if (sm != NULL)
		if (isdigit(sm[1]))
		  switch (sm[1])
		    {
		    case '1':
		      chmode = 1;
		      break;
		    case '2':
		      chmode = 2;
		      break;
		    case '4':
		      chmode = 4;
		      break;
		    }

	      switch (s[0])
		{
		case 'd':
		  if (chmode)
		    dmode = chmode;
		  for (j = 0; j < 8; j++)
		    {
		      printf("%02x:", p);
		      switch (dmode)
			{
			case 1:
			  {
			    unsigned char *pEvrbyte = (unsigned char *) pEvr;
			    for (i = 0; i < 16; i++)
			      printf(" %02x", pEvrbyte[p+i]);
			    break;
			  }
			case 2:
			  {
			    p &= -2;
			    unsigned short *pEvrshort =
			      (unsigned short *) pEvr;
			    for (i = 0; i < 8; i++)
			      printf(" %04x", be16_to_cpu(pEvrshort[p/2+i]));
			    break;
			  }
			case 4:	
			  p &= -4;
			  for (i = 0; i < 4; i++)
			    printf(" %08x", be32_to_cpu(pEvr[p/4+i]));
			  break;
			}
		      printf("\n");
		      p += 16;
		      p &= MEM_WINDOW - 1;
		    }
		  break;
		case 'm':
		  if (chmode)
		    mmode = chmode;
		  do
		    {
		      switch (mmode)
			{
			case 1:
			  {
			    unsigned char *pEvrbyte = (unsigned char *) pEvr;
			    printf("%08x: %02x ? ", p, pEvrbyte[p]);
			    fflush(stdout);
			    fgets(s, sizeof(s), stdin);
			    if (isxdigit(s[0]))
			      {
				d = strtoul(s, NULL, 16);
				pEvrbyte[p] = d;
			      }
			    p++;
			    break;
			  }
			case 2:
			  {
			    p &= -2;
			    unsigned short *pEvrshort =
			      (unsigned short *) pEvr;
			    printf("%08x: %04x ? ", p,
				   be16_to_cpu(pEvrshort[p/2]));
			    fflush(stdout);
			    fgets(s, sizeof(s), stdin);
			    if (isxdigit(s[0]))
			      {
				d = strtoul(s, NULL, 16);
				pEvrshort[p/2] = be16_to_cpu(d);
			      }
			    p += 2;
			    break;
			  }
			case 4:
			  p &= -4;
			  printf("%08x: %08x ? ", p & 0xfffffffc,
				 be32_to_cpu(pEvr[p/4]));
			  fflush(stdout);
			  fgets(s, sizeof(s), stdin);
			  if (isxdigit(s[0]))
			    {
			      d = strtoul(s, NULL, 16);
			      pEvr[p/4] = be32_to_cpu(d);
			    }
			  p += 4;
			  break;
			}
		    }
		  while (s[0] != '.');
		}
	    }
	  while (s[0] != 'q');

	  munmap(0, MEM_WINDOW);
	}
      close(fdEvr);
    }
}
