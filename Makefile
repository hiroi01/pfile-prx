TARGET = pfile

PSP_FW_VERSION = 500
BUILD_PRX = 1

OBJS =	\
	main.o \
	button.o \
	memory.o \
	config.o \
	filebrowser.o \
	file.o \
	log.o \
	exports.o \
	sceiomove.o

# syslibc
OBJS += syslibc/qsort.o
OBJS += syslibc/syslibc.o
OBJS += syslibc/strncasecmp.o

# threadctrl
OBJS += threadctrl/threadctrl.o

PRX_EXPORTS = exports.exp
USE_KERNEL_LIBC	= 1
USE_KERNEL_LIBS	= 1

CLASSG_LIBS = libs
#CLASSG_LIBS = libc

INCDIR = $(CLASSG_LIBS)
#INCDIR += include
CFLAGS = -O2 -G0 -Wall -fno-strict-aliasing -fno-builtin-printf
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

#LIBDIR = lib
LDFLAGS	= -mno-crt0 -nostartfiles
LIBS += -lcmlibMenu -linip -lpspkubridge -lpspsystemctrl_kernel  -lpsppower
#LIBS += syslibc/libsyslibc.a
#LIBS += threadctrl/libthreadctrl.a

ifdef DEBUG
	CFLAGS += -DDEBUG=$(DEBUG)
endif

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
