

#ifndef __CONF_H__
#define	__CONF_H__

#include <stdbool.h>

#define CONFIG_CONFIG_NUM 6
#define CONFIG_THEME_NUM 3

struct Pfile_Config {
	u32 start_key;
	char home_path[128];
	bool swap_xo;
	bool expert;
	char work_with_only[128];
	bool is_cef;
};

struct Pfile_Theme {
	u32 fg;
	u32 bg;
	u32 sl;
};

extern struct Pfile_Config g_config;
extern struct Pfile_Theme g_theme;

void setConfig(void);
int initConfig(const char *path);
int writeTheme(void);
int writeConfig(void);

#endif

