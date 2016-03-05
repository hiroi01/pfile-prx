TARGET_LIB = libsyslibc.a

OBJS += qsort.o
OBJS += syslibc.o
OBJS += strncasecmp.o

INCDIR		= 
CFLAGS 		= -O2 -G0 -Wall
CXXFLAGS 	= $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS 	= $(CFLAGS)

LIBDIR		= 
LDFLAGS		= 
LIBS		= 


PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
