// thanks to IO Move sample by Total_Noob

#include <pspkernel.h>
#include <string.h>

int sceIoMove(const char *src, const char *dest)
{
  size_t i;
  char strage[32];
  char *p1, *p2;
  p1 = strchr(src, ':');
  if (p1 == NULL) {
    return -1;
  }
  p2 = strchr(dest, ':');
  if (p2 == NULL) {
    return -1;
  }
  if ((p1-src) != (p2-dest)) {
    return -1;
  }

  for (i = 0; (src+i) <= p1; i++) {
    if ((i+1) >= sizeof(strage)) {
      return -1;
    }
    if (src[i] != dest[i]) {
      return -1;
    }
    strage[i] = src[i];
  }
  strage[i] = '\0';

	u32 data[2];
	data[0] = (u32)(p1+1);
	data[1] = (u32)(p2+1);
	int res = sceIoDevctl(strage, 0x02415830, &data, sizeof(data), NULL, 0);

	return res;
}
