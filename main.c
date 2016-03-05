/*
	pfile ver.0.01   by hiroi01
*/

#include <pspkernel.h>
#include <pspsdk.h>
#include <psputilsforkernel.h>
#include <pspsysmem_kernel.h>

#include <systemctrl.h>
#include <pspinit.h>

#include <psprtc.h>
#include <pspdisplay.h>
#include <pspsysevent.h>
#include <psppower.h>

#include <string.h>

#include <cmlibmenu.h>

#include "filebrowser.h"
#include "color.h"
#include "button.h"
#include "log.h"
#include "threadctrl.h"
#include "config.h"

/*-----------------------------------------------------*/

PSP_MODULE_INFO("pfile", PSP_MODULE_KERNEL, 0, 0);

/*-----------------------------------------------------------------*/

int loadCmlib(void);
extern int sceKernelCheckExitCallback();
int check_game_title(const char *title);

/*-----------------------------------------------------------------*/

int g_running;

libm_draw_info g_dinfo;
libm_vram_info g_vinfo;

static const char* init_file_name;

/*-----------------------------------------------------------------*/

#define PSP_SYSEVENT_SUSPEND_QUERY 0x100

int callbackSuspend(int ev_id, char *ev_name, void *param, int *result);

static PspSysEventHandler events =
{
	.size = sizeof(PspSysEventHandler),
	.name = "SysEventHandler",
	.type_mask = 0x0000FF00,
	.handler = callbackSuspend,
};

int callbackSuspend(int ev_id, char *ev_name, void *param, int *result)
{	
	if (ev_id == PSP_SYSEVENT_SUSPEND_QUERY){
		g_running = 0;
	}
	
	return 0;
}

/*-----------------------------------------------------------------*/

void mainMenu()
{
	g_running = 1;
		
	dbgprintf("suspend threads\n");
	threadCtrlSuspend();
	

	FBMain();
	
	dbgprintf("resume threads\n");
	threadCtrlResume();
	
	g_running = 0;
}

int main_thread(SceSize arglen, void *argp)
{
	SceCtrlData pad;

	while (sceKernelFindModuleByName("sceKernelLibrary") == NULL) {
		sceKernelDelayThread(100);
	}

#if DEBUG >= 1
	sceKernelDelayThread(5*1000*1000);
#endif
	dbgprintf("--- pfile debug log start ---\n");
	
	if (initConfig(argp) != 0) {
		dbgprintf("error: initConfig\n");
		return sceKernelExitDeleteThread(0);
	}

	if (g_config.is_cef) {
		threadCtrlSetSuspendMode(1);
	}

	//enable work_with_only option	
	if (g_config.work_with_only[0] != '\0') {
	   	if (check_game_title(g_config.work_with_only) != 0) {
			dbgprintf("work_with_only \"%s\", exit\n", g_config.work_with_only);
			return sceKernelExitDeleteThread(0);
		}
	}

	int res = loadCmlib();
	if (res < 0) {
		dbgprintf("cannot load cmlib\n");
		return sceKernelExitDeleteThread(0);
	}
	sceKernelDelayThread(5 * 100 * 1000);

	libmLoadFont(LIBM_FONT_SJIS);
	dbgprintf("load LIBM_FONT_SJIS\n");
	sceKernelDelayThread(5 * 100 * 1000);
	
	libmLoadFont(LIBM_FONT_CG);
	dbgprintf("load LIBM_FONT_CG\n");
	sceKernelDelayThread(5 * 100 * 1000);
	
	g_dinfo.vinfo = &g_vinfo;

	FBInit(g_config.home_path);	
	
	sceKernelRegisterSysEventHandler(&events);

	dbgprintf("start_key: %x\n", g_key_config[KEY_CONFIG_BOOT]);
	for (;;) {
		getButtons(&pad);

		if ((pad.Buttons & g_key_config[KEY_CONFIG_BOOT]) == g_key_config[KEY_CONFIG_BOOT]) {
			mainMenu();
		}

		sceKernelDelayThread(2 * 100 * 1000);
	}

	return 0;
}

int module_start(SceSize arglen, void *argp)
{
	SceUID thid = sceKernelCreateThread("pfile_main_thread", main_thread, 30, 0x1000, PSP_THREAD_ATTR_NO_FILLSTACK, 0 );
	if (thid < 0) {return thid;}

	init_file_name = sceKernelInitFileName();
	threadCtrlInit();
	
	sceKernelStartThread(thid, arglen, argp);

	return 0;
}

int module_stop( void )
{
	return 0;
}

int loadCmlib(void)
{
	int res = 0;
	char path[] = "ms0:/seplugins/lib/cmlibmenu.prx";
	
	if (sceKernelFindModuleByName("cmlibMenu") == NULL) {
		res = pspSdkLoadStartModule(path, PSP_MEMORY_PARTITION_KERNEL);
		dbgprintf("pspSdkLoadStartModule(%s): %#x\n", path, res);
		if (res < 0) {
			if (path[0] == 'm' && path[1] == 's') {
				path[0] = 'e';
				path[1] = 'f';
			} else {
				path[0] = 'm';
				path[1] = 's';
			}
			
			res = pspSdkLoadStartModule(path, PSP_MEMORY_PARTITION_KERNEL);
			dbgprintf("pspSdkLoadStartModule(%s): %#x\n", path, res);
		}
	}
	
	return res;
}


/*-----------------------------------------------------------------*/

static unsigned char ebootpbp_signature[] = { 0x00, 0x50, 0x42, 0x50 };
static unsigned char paramsfo_magic[] = { 0x00, 0x50, 0x53, 0x46 };

typedef struct ebootpbp_header_{
	u8 signature[4];
	u32 version;
	u32 offset[8];
}ebootpbp_header;

typedef struct psf_header_{
	u8 magic[4];
	u8 rfu000[4];
	u32 label_ptr;
	u32 data_ptr;
	u32 nsects;
}psf_header;

typedef struct psf_section_{
	u16 label_off;
	u8 rfu001;
	u8 data_type;
	u32 datafield_used;
	u32 datafield_size;
	u32 data_off;
}psf_section;

int readGameTitle(int fd, char *rtn, int rtnSize)
{
	ebootpbp_header pbpHeader;
	psf_header header;
	psf_section section;
	int i;
	off_t currentPosition;
	off_t offset = 0;
	char label[6];//TITLE[EOS]
	
	offset = sceIoLseek(fd, 0, PSP_SEEK_CUR);
	sceIoRead(fd, &pbpHeader, sizeof(ebootpbp_header));
	if (memcmp(&pbpHeader.signature, ebootpbp_signature, sizeof(ebootpbp_signature)) == 0) {
		offset += pbpHeader.offset[0];
	}
	
	sceIoLseek(fd, offset, PSP_SEEK_SET);
	sceIoRead(fd, &header, sizeof(psf_header));
	if (memcmp(&header.magic, paramsfo_magic, sizeof(paramsfo_magic)) != 0) {
		return -1;
	}
	
	currentPosition = sceIoLseek(fd, 0 ,PSP_SEEK_CUR);
	for (i = 0; i < header.nsects; i++) {
		sceIoLseek(fd, currentPosition, PSP_SEEK_SET);
		sceIoRead(fd, &section, sizeof(psf_section));
		currentPosition = sceIoLseek(fd, 0 ,PSP_SEEK_CUR);
		
		sceIoLseek(fd, offset + header.label_ptr + section.label_off, PSP_SEEK_SET);
		sceIoRead(fd, label, sizeof(label)-1);
		label[sizeof(label)-1] = '\0';
		
		if (strcmp(label, "TITLE") == 0) {
			sceIoLseek(fd, offset + header.data_ptr + section.data_off, PSP_SEEK_SET);
			sceIoRead(fd, rtn, rtnSize - 1);
			rtn[rtnSize - 1] = '\0';
			return 0;
		}
	}
	
	//not found "TITLE" section
	return 1;
}

int check_game_title(const char *title)
{
    char str[128];
	int res;
    SceUID fd;
	
	dbgprintf("init_file_name: %s\n", init_file_name);
	if (init_file_name == NULL) {
		return 1;
	}
		
	fd = sceIoOpen(init_file_name, PSP_O_RDONLY, 0777);
    if (fd < 0) {
		return fd;
	}

    res = readGameTitle(fd, str, 128);
    sceIoClose(fd);

	dbgprintf("readGameTitle(): %d\n", res);
	
    if (res != 0) {
		return 2;
	}

	dbgprintf("game_title: \"%s\"\n", str);
	if ((strcmp(str, title)) != 0) {
        return 3;
    }

	return 0;
}


