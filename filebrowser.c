#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspsysmem_kernel.h>

#include <stdbool.h>
#include <string.h>

#include <cmlibmenu.h>

#include "main.h"
#include "file.h"
#include "config.h"
#include "filebrowser.h"
#include "button.h"
#include "memory.h"
#include "sceiomove.h"

#include "log.h"

#define IS_SJIS_1BYTE_CHAR(c) (((unsigned char)c <= 0x80) || ((unsigned char)c >= 0xA1 && (unsigned char)c <= 0xDF))

#define DIR_BUF_NUM (256)
#define PATH_LEN (128)

#define MAX_DISPLAY_NUM (24)
#define START_X (4)
#define START_Y (18)

static void FBcd(const char *path, int mode);

typedef struct File_Browser_Info_{
	dir_t dir[DIR_BUF_NUM];
	int dir_num;
	int arrow_pos;
	int start_pos;
	char current_path[PATH_LEN];
	struct {
		char path[PATH_LEN];
		dir_t file;
		int mode;
	} cb;
}File_Browser_Info;

static File_Browser_Info fb_data;

static void clearScreen(void)
{
	libmFillRect(0, 0, 480, 272, g_theme.bg, &g_dinfo);
}

static void makeWindow(int start_x, int start_y, int end_x, int end_y, u32 fg, u32 bg, unsigned int delay_time)
{
	if (delay_time == 0) {
		libmFillRect(start_x , start_y , end_x , end_y , bg, &g_dinfo);
		libmFrame(start_x , start_y , end_x , end_y , fg, &g_dinfo);
	}

	int x = start_x, y = start_y;
	while (g_running) {
		x += 8, y += 8;
		if (x > end_x) {
			x = end_x;
		}
		if (y > end_y) {
			y = end_y;
		}

		libmFillRect(start_x , start_y , x , y , bg, &g_dinfo);
		libmFrame(start_x , start_y , x ,y , fg, &g_dinfo);

		if (x == end_x && y == end_y) {
			break;
		}

		sceKernelDelayThread(delay_time);
	}
}

static inline void _libmPutChar(int x, int y, u32 fg, u32 bg, const char *str)
{
	char ch[3] = { str[0], '\0', '\0' };

	//is 2byte char
	if (!IS_SJIS_1BYTE_CHAR(str[0])) {
		ch[1] = str[1];
	}

	libmPrintXY(x, y, fg, bg, ch, &g_dinfo);
}

#define PRINT_BUF_SIZE (((480 - START_X)/8)*2+1)

static inline void _libmPrint(int x, int y, u32 fg, u32 bg, const char *str)
{
	unsigned int i;
	unsigned int len = strlen(str);
	for (i = 0; i < len; i++) {
		_libmPutChar(x+i*8, y, fg, bg, &str[i]);
	}
	libmFillRect(x+i*8, y, 480, y+8, bg, &g_dinfo);
}

static inline void _libmPrintf(int x, int y, u32 fg, u32 bg, const char* format, ...)
{
	va_list ap;
	char buf[PRINT_BUF_SIZE];

	va_start(ap, format);
	vsnprintf(buf, PRINT_BUF_SIZE, format, ap);
	va_end(ap);
	
	_libmPrint(x, y, fg, bg, buf);
}

static inline void _libmPrintf2(int x, int y, u32 fg, u32 bg, const char* format, ...)
{
	va_list ap;
	char buf[PRINT_BUF_SIZE];

	va_start(ap, format);
	vsnprintf(buf, PRINT_BUF_SIZE, format, ap);
	va_end(ap);
	
	libmPrintXY(x, y, fg, bg, buf, &g_dinfo);
}

static void drawHeader(const char* str)
{
	unsigned int len = strlen(str);
	const char* ptr = str;
	if (len > 42) {
		ptr += len - 42;
	}
	libmPrintXY(4, 2, (g_config.expert)?g_theme.sl:g_theme.fg, g_theme.bg, "pfile by hiroi01", &g_dinfo);
	libmPrintXY(4+8*17, 2, g_theme.fg, g_theme.bg, ptr, &g_dinfo);
	libmFillRect(134, 0, 135, 12, g_theme.fg, &g_dinfo);
	libmFillRect(0, 12, 480, 13, g_theme.fg, &g_dinfo);
}

static void drawHeader2(void)
{
	char *str = "pfile ver.0.01   by hiroi01";
	unsigned int len = sizeof(str) - 1;

	libmFillRect(0, 0, 480, 12, g_theme.bg, &g_dinfo);
	_libmPrint((272-(8*len))/2, 2, (g_config.expert)?g_theme.sl:g_theme.fg, g_theme.bg, str);
}

enum {
	FOOTER_MAIN_MENU = 0,
	FOOTER_FILE_MENU,
	FOOTER_CONFIG_MENU,
	FOOTER_COPYING,
	FOOTER_REMOVE_MENU,
};

static void drawFooter(int num)
{
	char *message[] = {
		"%s:menu %s:exit R-TRI:up_folder L-TRI:down_folder START:reload",
		"%s:select %s:cancel",
		"%s:close %s:close",
		"%s:_ LongPress%s:cancel",
		"%s:select %s:cancel",
	};

	char *ok_key_name = "X", *cancel_key_name = "O";
	if (g_config.swap_xo) {
		ok_key_name = "O";
		cancel_key_name = "X";
	}

	_libmPrintf(0, 272-8, g_theme.fg, g_theme.bg, message[num], ok_key_name, cancel_key_name);
	libmFillRect(0, 272-11, 480, 272-10, g_theme.fg, &g_dinfo);
}

static inline void drawRemoveFile(int pos, int x, int y)
{
	u32 no_color, yes_color;
	if (pos == 0) {
		no_color = g_theme.sl;
		yes_color = g_theme.fg;
	} else {
		no_color = g_theme.fg;
		yes_color = g_theme.sl;
	}

	libmPrintXY(x+8*1, y+4+8*1, no_color, g_theme.bg, "no", &g_dinfo);
	libmPrintXY(x+8*4, y+4+8*1, yes_color, g_theme.bg, "yes", &g_dinfo);
}


static int removeFile(const char *file, int x, int y)
{
	makeWindow(x, y, x+8*2+8*10, y+8*2+8*1, g_theme.fg, g_theme.bg, 8*1000);
	libmPrintXY(x+8, y, g_theme.fg, g_theme.bg, "delete OK?", &g_dinfo);
	
	SceCtrlData pad;
	int res = 0, pos = 0;
	while (g_running && res == 0) {
		sceKernelDelayThread(1);
		sceDisplayWaitVblankStart();
		drawRemoveFile(pos, x, y);

		getButtons(&pad);
		if (pad.Buttons & (PSP_CTRL_RIGHT|PSP_CTRL_LEFT)) {
			pos ^= 1;
			waitButtonsRelease();
		} else if (pad.Buttons & g_key_config[KEY_CONFIG_OK]) {
			res = 1;
		} else if (pad.Buttons & g_key_config[KEY_CONFIG_CANCEL]) {
			res = -1;
		}
	}

	if (res > 0 && pos != 0) {
		int res = sceIoRemove(file);
		dbgprintf("sceIoRemove(%s): %x\n", file, res);

		if (res != 0) {
			makeWindow(x+4, y+4, x+4+8*2+8*12, y+4+8*2+8*1, g_theme.fg, g_theme.bg, 8*1000);
			libmPrintXY(x+4+8, y+4+8, g_theme.fg, g_theme.bg, "delete error", &g_dinfo);
			waitButtonsRelease();
			waitPressAnyButtons();
		}

		return res;
	}
	
	return 1;
}

static int copyFile(const char* file1, const char* file2)
{
	int res = 0;
	SceUID fdr = -1, fdw = -1;
	char* buf = NULL;
	unsigned int buf_size;
	SceIoStat stat;

	makeWindow(24, 30, 24+8*2+8*24, 30+8*2+8*3, g_theme.fg, g_theme.bg, 8*1000);
	libmPrintXY(24+8, 30+8, g_theme.fg, g_theme.bg, "copying...", &g_dinfo);
	drawFooter(FOOTER_COPYING);

	if (check_file(file2) == 0) {
		makeWindow(28, 38, 28+8*2+8*24, 38+8*2+8*1, g_theme.fg, g_theme.bg, 8*1000);
		libmPrintXY(28+8, 38+8, g_theme.fg, g_theme.bg, "file exists", &g_dinfo);

		waitPressAnyButtons();
		return 2;
	}

	if (sceIoGetstat(file1, &stat) < 0) {
		res = -1;
		goto EXIT;
	}

	fdr = sceIoOpen(file1, PSP_O_RDONLY, 0777);
	if (fdr < 0) {
		res = -2;
		goto EXIT;
	}
	fdw = sceIoOpen(file2, PSP_O_WRONLY|PSP_O_CREAT|PSP_O_TRUNC, 0777);
	if (fdw < 0) {
		res = -3;
		goto EXIT;
	}

	buf_size =  25 * 1024 * 1024;
   	for (;;) {
		buf	= mem_alloc(buf_size);
		if (buf != NULL) {
			break;
		}
		buf_size -= 1024;
		if (buf_size <= 0) {
			res = -4;
			goto EXIT;
		}
	}
	dbgprintf("buf_size: %d\n", buf_size);

	SceCtrlData pad;	
	int read_size, write_size;
	unsigned int total_size = 0;
	unsigned int file_size_kb = stat.st_size / 1024;
	for (;;) {
		_libmPrintf2(24+8, 30+8*3, g_theme.fg, g_theme.bg, "%9d/%9d [kB]", total_size/1024, file_size_kb, &g_dinfo);

		if (total_size == stat.st_size) {
			break;
		}

		getButtons(&pad);
		if (pad.Buttons & g_key_config[KEY_CONFIG_CANCEL]) {
			res = 1;
			goto EXIT;
		}

		read_size = sceIoRead(fdr, buf, buf_size);
		if (read_size < 0) {
			res = -5;
			goto EXIT;
		}

		total_size += read_size;
		if (read_size < buf_size && total_size != stat.st_size) {
			res = -6;
			goto EXIT;
		}
		
		write_size = sceIoWrite(fdw, buf, read_size);
		if (read_size != write_size) {
			res = -7;
			goto EXIT;
		}
	}
	

EXIT:
	if (fdr >= 0) {
		sceIoClose(fdr);
	}
	if (fdw >= 0) {
		sceIoClose(fdw);
	}
	if (buf != NULL) {
		mem_free(buf);
	}

	if (res == 0) {
		SceIoStat dst_stat;
		sceIoGetstat(file2, &dst_stat);
		if (stat.st_size != dst_stat.st_size) {
			res = -8;
		}
	}

	if (res != 0) {
		sceIoRemove(file2);
		
		if (res < 0) {
			makeWindow(28, 34, 28+8*2+8*9, 34+8*2+8*2, g_theme.fg, g_theme.bg, 8 * 1000);
			_libmPrintf2(28+8, 34+8, g_theme.fg, g_theme.bg, "error: %d", res);
		} else {
			makeWindow(28, 34, 28+8*2+8*9, 34+8*2+8*2, g_theme.fg, g_theme.bg, 8 * 1000);
			libmPrintXY(28+8, 34+8, g_theme.fg, g_theme.bg, "canceled", &g_dinfo);
		}

		waitButtonsRelease();
		waitPressAnyButtons();
	}


	return res;
}

static int moveFile(const char *file1, const char *file2)
{
	int res;

	res = sceIoMove(file1, file2);
	if (res < 0) {
		res = copyFile(file1, file2);
		if (res == 0) {
			res = sceIoRemove(file1);
			if (res == 0) {
				fb_data.cb.mode = -1;
				return 0;
			}
		}

		makeWindow(28, 34, 28+8*2+8*9, 34+8*2+8*2, g_theme.fg, g_theme.bg, 8 * 1000);
		_libmPrintf2(28+8, 34+8, g_theme.fg, g_theme.bg, "error: %d", res);

		waitButtonsRelease();
		waitPressAnyButtons();
	} else {
		fb_data.cb.mode = -1;
	}

	return res;
}

static inline void drawSubMenu(int x, int y, char *item[], int pos)
{
	int i;

	//makeWindow(x, y, end_x, end_y, g_theme.fg, g_theme.bg, 0);
	for (i = 0; item[i] != NULL; i++) {
		libmPrintXY(x, y+8*i, (pos == i)?g_theme.sl:g_theme.fg, g_theme.bg, item[i], &g_dinfo);
	}
}

static inline int buttonsBranchSubMenu(u32 current_buttons, u32 previous_buttons, int *pos, int item_num)
{
	if (detectButtons(PSP_CTRL_DOWN, current_buttons, previous_buttons, 3*100*1000, 1*100*1000)) {
		if ((*pos)+1 < item_num) {
			(*pos)++;
		} else {
		   *pos = 0;
		}	   
	} else if (detectButtons(PSP_CTRL_UP, current_buttons, previous_buttons, 3*100*1000, 1*100*1000)) {
		if ((*pos)-1 >= 0) {
			(*pos)--;
		} else {
		   *pos = item_num - 1;
		}	   
	} else if (current_buttons & g_key_config[KEY_CONFIG_OK]) {
		return 1;
	} else if (current_buttons & g_key_config[KEY_CONFIG_CANCEL]) {
		return 2;
	}

	return 0;
}

static int subMenu(int start_x, int start_y, int end_x, int end_y, char *item[])
{
	SceCtrlData pad;
	u32 previous_buttons = 0;
	int pos = 0;
	int item_num;

	for (item_num = 0; item[item_num] != NULL; item_num++);

	makeWindow(start_x, start_y, end_x, end_y, g_theme.fg, g_theme.bg, 8 * 1000);

	while (g_running) {
		getButtons(&pad);
		int res = buttonsBranchSubMenu(pad.Buttons, previous_buttons, &pos, item_num);
		switch (res) {
			case 0:
				//DO NOTHING
				break;
			case 1:
				return pos;
			case 2:
				return -1;
		}
		previous_buttons = pad.Buttons;

		sceKernelDelayThread(1);
		sceDisplayWaitVblankStart();
		drawSubMenu(start_x+8, start_y+8, item, pos);
	}

	return -1;
}

static void configMenu()
{
	char *str = "not implemented";
	unsigned int len = strlen(str);

	drawHeader2();
	drawFooter(FOOTER_CONFIG_MENU);

	makeWindow(24, 34, 24+8*2+8*len, 34+8*2+8*1, g_theme.fg, g_theme.bg, 10*1000);
	libmPrintXY(24+8, 34+8*1, g_theme.fg, g_theme.bg, str, &g_dinfo);

	waitButtonsRelease();	
	waitPressAnyButtons();
	SceCtrlData pad;
	getButtons(&pad);
	if ((pad.Buttons&(PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER)) == (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER)) {
		g_config.expert = true;
		makeWindow(28, 38, 28+8*2+8*20, 38+8*2+8*2, g_theme.bg, g_theme.fg, 10*1000);
		libmPrintXY(28+8, 38+8*1, g_theme.bg, g_theme.fg, "expert mode", &g_dinfo);
		libmPrintXY(28+8, 38+8*2, g_theme.bg, g_theme.fg, "use at your own risk", &g_dinfo);
		waitButtonsRelease();	
		waitPressAnyButtons();
	}
}

static void selectStrage()
{
	char *menu_item_default[] = {
		"ms0:/",
		"config",
		NULL,
	};
	char *menu_item_go[] = {
		"ms0:/",
		"ef0:/",
		"config",
		NULL,
	};
	char *menu_item_expert[] = {
		"ms0:/",
		"disk0:/",
		"umd0:/",
		"flash0:/",
		"flash1:/",
		"flash2:/",
		"flash3:/",
		"config",
		NULL,
	};
	char *menu_item_expert_go[] = { 
		"ms0:/",
		"ef0:/",
		"umd0:/",
		"disk0:/",
		"flash0:/",
		"flash1:/",
		"flash2:/",
		"flash3:/",
		"eh0:/",
		"config",
		NULL,
	};
	menu_item_go[0][0] = 'm';
	menu_item_go[0][1] = 's';
	menu_item_expert_go[0][0] = 'm';
	menu_item_expert_go[0][1] = 's';

	char **menu_item;
	int menu_item_num;

	if (sceKernelGetModel() == 4) {
		if (g_config.expert) {
			menu_item = menu_item_expert_go;
			menu_item_num = (sizeof(menu_item_expert_go) / sizeof(char *)) - 1;
		} else {
			menu_item = menu_item_go;
			menu_item_num = (sizeof(menu_item_go) / sizeof(char *)) - 1;
		}
	} else {
		if (g_config.expert) {
			menu_item = menu_item_expert;
			menu_item_num = (sizeof(menu_item_expert) / sizeof(char *)) - 1;
		} else {
			menu_item = menu_item_default;
			menu_item_num = (sizeof(menu_item_default) / sizeof(char *)) - 1;
		}
	}

	int res = subMenu(20, 30, 96, 46+8*menu_item_num, menu_item);

	if (res < 0) {
		return;
	}
	if (res == menu_item_num-1) {
		configMenu();
		return;
	}

	FBcd(menu_item[res], 0);
	return;
}

#define FILE_MENU_Y_SIZE (48)
#define FILE_MENU_START_X (10)
#define FILE_MENU_X_SIZE (76)

static inline int fileMenu(char *current_path, dir_t *file, int y, int mode)
{
	int res;
	char *menu_item[] = {
		"copy",
		"cut",
		"delete",
		"paste",
		NULL,
	};
	char target_full_path[256];
	char cb_full_path[256] = "";

	if (fb_data.cb.mode >= 0) {
		snprintf(cb_full_path, 256, "%s%s", fb_data.cb.path, fb_data.cb.file.name);
	}

	drawHeader(cb_full_path);
	drawFooter(FOOTER_FILE_MENU);

	while (g_running) {
		int menu_res = subMenu(FILE_MENU_START_X, y, FILE_MENU_START_X+FILE_MENU_X_SIZE, y+FILE_MENU_Y_SIZE, menu_item);
		waitButtonsRelease();
		switch (menu_res) {
			case 0:
				if (mode != 0) {
					break;
				}

				strcpy(fb_data.cb.path, current_path);
				fb_data.cb.file = *file;
				fb_data.cb.mode = 0;
				return 0;
			case 1:
				if (mode != 0) {
					break;
				}

				strcpy(fb_data.cb.path, current_path);
				fb_data.cb.file = *file;
				fb_data.cb.mode = 1;
				return 0;
			case 2:
				if (mode != 0) {
					break;
				}

				snprintf(target_full_path, 256, "%s%s", current_path, file->name);
				removeFile(target_full_path, FILE_MENU_START_X+4, y+4);
				return 1;
			case 3:
				if (fb_data.cb.mode < 0) {
					break;
				}
				
				snprintf(target_full_path, 256, "%s%s", current_path, fb_data.cb.file.name);
				if (fb_data.cb.mode == 0) {
					res = copyFile(cb_full_path, target_full_path);
				} else {
					res = moveFile(cb_full_path, target_full_path);
				}
				return (res == 0)?1:0;
			default:
				return 0;
		}
	}

	return 0;
}

void drawFileBrowser(void)
{
	if (fb_data.dir_num == 0) {
		return;
	}

	int i = ((fb_data.dir_num < MAX_DISPLAY_NUM) ? fb_data.dir_num : MAX_DISPLAY_NUM) - 1;
	int j = fb_data.start_pos + i;
	for (; i >= 0; i--, j--) {
		if (fb_data.dir[j].type == TYPE_DIR) {
			if (j == fb_data.arrow_pos) {
				_libmPrintf(START_X, START_Y + (LIBM_CHAR_HEIGHT+2)*i, g_theme.sl, g_theme.bg, "%s/", fb_data.dir[j].name);
			} else {
				_libmPrintf(START_X, START_Y + (LIBM_CHAR_HEIGHT+2)*i, g_theme.fg, g_theme.bg, "%s/", fb_data.dir[j].name);
			}
		} else {
			if (j == fb_data.arrow_pos) {
				_libmPrintf(START_X, START_Y + (LIBM_CHAR_HEIGHT+2)*i, g_theme.sl, g_theme.bg, "%s", fb_data.dir[j].name);
			} else {
				_libmPrintf(START_X, START_Y + (LIBM_CHAR_HEIGHT+2)*i, g_theme.fg, g_theme.bg, "%s", fb_data.dir[j].name);
			}
		}
	}
}

static void FBcd(const char *path, int mode)
{
	if (mode == 0) {
		strcpy(fb_data.current_path, path);
	} else {
		strcat(fb_data.current_path, path);
		char *ptr = strchr(fb_data.current_path, '\0');
		if (ptr[-1] != '/') {
			ptr[0] = '/';
			ptr[1] = '\0';
		}
	}
}

static inline int aliasProcess(const char *current_path, const char *file_name)
{
	char *extension = strchr(file_name, '\0') - sizeof(ALIAS_EXTENSION) + 1;
	if (extension < file_name || strcasecmp(extension, ALIAS_EXTENSION) != 0) {
		return 1;
	}
	
	char path[PATH_LEN];
	char buf[PATH_LEN];
	char *ptr;

	strcpy(path, current_path);
	strcat(path, file_name);

	SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
	if (fd < 0) {
		return 1;
	}
	SceSize size = sceIoRead(fd, buf, PATH_LEN - 1);
	sceIoClose(fd);

	buf[size] = '\0';
	ptr = strchr(buf, '\r');
	if (ptr != NULL) {
		*ptr = '\0';
	}
	ptr = strchr(buf, '\n');
	if (ptr != NULL) {
		*ptr = '\0';
	}

	FBcd(path, 0);

	return 0;
}

enum {
	MAIN_MENU_KEEP = 0,
	MAIN_MENU_REFRESH,
	MAIN_MENU_READDIR,
	MAIN_MENU_EXIT,
};

static inline int buttonsBranchMainMenu(u32 current_buttons, u32 previous_buttons)
{
	if (current_buttons & g_key_config[KEY_CONFIG_OK]) {
		int pos, y, mode, res;

		pos = fb_data.arrow_pos - fb_data.start_pos + 1;
		y = (pos < 17)?(START_Y+pos*10+2):(START_Y+pos*10-FILE_MENU_Y_SIZE-16);
		mode = (fb_data.dir_num == 0)?1:0;

		waitButtonsRelease();
		res = fileMenu(fb_data.current_path, &fb_data.dir[fb_data.arrow_pos], y, mode);
		waitButtonsRelease();

		if (res != 0) {
			return MAIN_MENU_READDIR;
		}
		return MAIN_MENU_REFRESH;
	}
	if (current_buttons & PSP_CTRL_LTRIGGER) {
		char *ptr = strchr(fb_data.current_path, '\0') - 2;
		if (ptr != NULL && strcmp(ptr, ":/") == 0) { //is root dir
			selectStrage();
			return MAIN_MENU_READDIR;
		}
		if (up_dir(fb_data.current_path) == 0) {
			return MAIN_MENU_READDIR;
		}
		return MAIN_MENU_KEEP; 
	}
	if (current_buttons & PSP_CTRL_START) {
		return MAIN_MENU_READDIR;
	}
	if (current_buttons & PSP_CTRL_SELECT) {
		FBcd(g_config.home_path, 0);
		return MAIN_MENU_READDIR;
	}
	if (current_buttons & g_key_config[KEY_CONFIG_CANCEL]) {
		return MAIN_MENU_EXIT;
	}

	if (fb_data.dir_num == 0) {
		return MAIN_MENU_KEEP;
	}

	if (detectButtons(PSP_CTRL_DOWN, current_buttons, previous_buttons, 3*100*1000, 1*100*1000)) {
		if (fb_data.arrow_pos + 1 < fb_data.dir_num) {
			fb_data.arrow_pos++;
			if (fb_data.arrow_pos - fb_data.start_pos >= MAX_DISPLAY_NUM){
				fb_data.start_pos++;
			}
		} 
		return MAIN_MENU_KEEP; 
	}
	if (detectButtons(PSP_CTRL_UP, current_buttons, previous_buttons, 3*100*1000, 1*100*1000)) {
		if (fb_data.arrow_pos - 1 >= 0) {
			fb_data.arrow_pos--;
			if(fb_data.arrow_pos < fb_data.start_pos) {
				fb_data.start_pos--;
			}
		}
		return MAIN_MENU_KEEP;
	}
	if (detectButtons(PSP_CTRL_RIGHT, current_buttons, previous_buttons, 3*100*1000, 1*100*1000)) {
		if (fb_data.arrow_pos + 5 < fb_data.dir_num) {
			fb_data.arrow_pos += 5;
		} else {
			fb_data.arrow_pos = fb_data.dir_num - 1;
		}
		if (fb_data.arrow_pos >= fb_data.start_pos + MAX_DISPLAY_NUM) {
			fb_data.start_pos = fb_data.arrow_pos - MAX_DISPLAY_NUM + 1;
		}
		return MAIN_MENU_KEEP;
	}
	if (detectButtons(PSP_CTRL_LEFT, current_buttons,  previous_buttons, 3*100*1000, 1*100*1000)) {
		if (fb_data.arrow_pos - 5 >= 0) {
			fb_data.arrow_pos -= 5;
		} else {
			fb_data.arrow_pos = 0;
		}
		if(fb_data.arrow_pos < fb_data.start_pos) {
			fb_data.start_pos = fb_data.arrow_pos;
		}
		return MAIN_MENU_KEEP;
	}
	if (current_buttons & PSP_CTRL_RTRIGGER) {
		if (fb_data.dir[fb_data.arrow_pos].type == TYPE_DIR) {
			FBcd(fb_data.dir[fb_data.arrow_pos].name, 1);
			return MAIN_MENU_READDIR;
		}
		if (aliasProcess(fb_data.current_path, fb_data.dir[fb_data.arrow_pos].name) == 0) {
			return MAIN_MENU_READDIR;
		}
		return MAIN_MENU_KEEP; 
	}

	return MAIN_MENU_KEEP; 
}

static void readDir(void)
{
	fb_data.start_pos = 0;
	fb_data.arrow_pos = 0;
			
	dbgprintf("read_dir(%s)\n", fb_data.current_path);
	fb_data.dir_num = read_dir(fb_data.dir, DIR_BUF_NUM, fb_data.current_path, 0, dir_type_sort_default);         
	dbgprintf("dir_num: %d\n", fb_data.dir_num);
}

static inline void refreshMainMenu(void)
{
	clearScreen();
	drawHeader(fb_data.current_path);
	drawFooter(FOOTER_MAIN_MENU);
}

void FBMain(void)
{	
	SceCtrlData pad;
	u32 previous_buttons = 0;

	while (!libmInitBuffers(LIBM_DRAW_INIT8888, PSP_DISPLAY_SETBUF_NEXTFRAME, &g_dinfo)) {
		sceKernelDelayThread(100);
	}

	if (fb_data.dir_num == 0) {
		readDir();
	}

	refreshMainMenu();
	drawFileBrowser();
	waitButtonsRelease();

	while (g_running) {
		sceKernelDelayThread(1);
		sceDisplayWaitVblankStart();
		drawFileBrowser();

		getButtons(&pad);
		int res = buttonsBranchMainMenu(pad.Buttons, previous_buttons);
		switch (res) {
			case MAIN_MENU_KEEP:
				//DO NOTHING
				break;
			case MAIN_MENU_REFRESH:
				refreshMainMenu();
				break;
			case MAIN_MENU_READDIR:
				waitButtonsRelease();
				readDir();
				refreshMainMenu();
				break;
			case MAIN_MENU_EXIT:
				waitButtonsRelease();
				return;
		}

		previous_buttons = pad.Buttons;
	}
}

void FBInit(const char *current_path)
{
	FBcd(current_path, 0);
	fb_data.dir_num = 0;
}

