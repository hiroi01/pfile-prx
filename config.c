#include <pspkernel.h>
#include <pspctrl.h>
#include <string.h>
#include <stdio.h>

#include <libinip.h>

#include "config.h"
#include "button.h"
#include "color.h"
#include "log.h"

#define CONFIG_CONFIG_NAME "pfile_ini.txt"
#define CONFIG_THEME_NAME "pfile_theme.txt"

struct Pfile_Config g_config;
struct Pfile_Theme g_theme;

static char config_path[64];

void setConfig(void)
{
	if (g_config.swap_xo) {
		g_key_config[KEY_CONFIG_OK] = PSP_CTRL_CIRCLE;
		g_key_config[KEY_CONFIG_CANCEL] = PSP_CTRL_CROSS;
	} else {
		g_key_config[KEY_CONFIG_OK] = PSP_CTRL_CROSS;
		g_key_config[KEY_CONFIG_CANCEL] = PSP_CTRL_CIRCLE;
	}
	g_key_config[KEY_CONFIG_BOOT] = g_config.start_key;

	char *ptr = strchr(g_config.home_path, '\0') - 1;
	if (ptr >= g_config.home_path) {
	   	if (*ptr != '/') {
			strcat(g_config.home_path, "/");
		}
	}

}

static int configFunction(const char *path, int mode)
{
	int res;
	
	ILP_Key key[CONFIG_CONFIG_NUM];
	res = ILPInitKey(key);
	if (res != 0) {
		dbgprintf("error: ILPInitKey\n");
		return res;
	}

	ILPRegisterButton(key, "start_key", &g_config.start_key, PSP_CTRL_LTRIGGER|PSP_CTRL_NOTE, NULL);
	ILPRegisterString(key, "home_path", g_config.home_path, "ms0:/");
	ILPRegisterBool(key, "swap_xo", &g_config.swap_xo, false);
	ILPRegisterBool(key, "expert", &g_config.expert, false);
	ILPRegisterString(key, "work_with_only", g_config.work_with_only, "");
	ILPRegisterBool(key, "is_cef", &g_config.is_cef, false);

	if (mode == 0) {
		res = ILPReadFromFile(key, path);
		setConfig();

		if (res != 0) {
			dbgprintf("error: ILPReadFromFile\n");
			return res;
		}
	} else {
		res = ILPWriteToFile(key, path, "\r\n");
		if (res != 0) {
			dbgprintf("error: ILPWriteFromFile\n");
			return res;
		}
	}
	
	return 0;
}

static int themeFunction(const char *path, int mode)
{
	int res;
	
	ILP_Key key[CONFIG_THEME_NUM];
	res = ILPInitKey(key);
	if (res != 0) {
		dbgprintf("error: ILPInitKey\n");
		return res;
	}
	
	ILPRegisterHex(key, "foreground_color", &g_theme.fg, WHITE);
	ILPRegisterHex(key, "background_color", &g_theme.bg, BLACK);
	ILPRegisterHex(key, "select_color", &g_theme.sl, YELLOGREEN);

	if (mode == 0) {
		res = ILPReadFromFile(key, path);
		if (res != 0) {
			dbgprintf("error: ILPReadFromFile\n");
			return res;
		}
	} else {
		res = ILPWriteToFile(key, path, "\r\n");
		if (res != 0) {
			dbgprintf("error: ILPWriteFromFile\n");
			return res;
		}
	}
	
	return 0;
}

static inline int loadConfig(const char *path)
{
	dbgprintf("loadConfig(%s)\n", path);
	return configFunction(path, 0);
}

static inline int saveConfig(const char *path)
{
	dbgprintf("saveConfig(%s)\n", path);
	return configFunction(path, 1);
}

static inline int loadTheme(const char *path)
{
	dbgprintf("loadTheme(%s)\n", path);
	return themeFunction(path, 0);
}

static inline int saveTheme(const char *path)
{
	dbgprintf("saveTheme(%s)\n", path);
	return themeFunction(path, 1);
}

int initConfig(const char *argp)
{
	char path[128];

	strcpy(config_path, argp);
	char *ptr = strrchr(config_path, '/');
	if (ptr != NULL) {
		ptr[1] = '\0';
	} else {
		config_path[0] = '\0';
	}

	snprintf(path, 128, "%s%s", config_path, CONFIG_CONFIG_NAME);
	loadConfig(path);

	snprintf(path, 128, "%s%s", config_path, CONFIG_THEME_NAME);
	loadTheme(path);

	return 0;
}

int writeConfig(void)
{
	char path[128];
	snprintf(path, 128, "%s%s", config_path, CONFIG_CONFIG_NAME);
	return saveConfig(path);
}

int writeTheme(void)
{
	char path[128];
	snprintf(path, 128, "%s%s", config_path, CONFIG_THEME_NAME);
	return saveTheme(path);
}

