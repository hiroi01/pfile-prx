#if DEBUG == 2

#include <pspkernel.h>

#include <stdio.h>
#include <string.h>

#define LOG_PATH "ms0:/pfile_log.txt"
#define LOG_BUF_SIZE 256

void logToFile(const char* format, ...)
{
	SceUID fd = sceIoOpen(LOG_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
	if(fd < 0) {
		return;
	}

	va_list ap;
	char buf[LOG_BUF_SIZE];

	va_start(ap, format);
	vsnprintf(buf, LOG_BUF_SIZE, format, ap);
	va_end(ap);

	sceIoWrite(fd, buf, strlen(buf));
	sceIoClose(fd);
}

#endif

