#ifndef __BUTTONS__
#define __BUTTONS__

#include <stdbool.h>
#include <pspctrl.h>
#include <pspkernel.h>

#define CHECK_KEY (PSP_CTRL_SELECT | PSP_CTRL_START | PSP_CTRL_UP | PSP_CTRL_RIGHT | \
    PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_TRIANGLE | \
    PSP_CTRL_CIRCLE | PSP_CTRL_CROSS | PSP_CTRL_SQUARE | PSP_CTRL_NOTE | PSP_CTRL_HOME)

#define CHECK_KEY_2 (PSP_CTRL_SELECT | PSP_CTRL_START | PSP_CTRL_UP | PSP_CTRL_RIGHT | \
    PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_TRIANGLE | \
    PSP_CTRL_CIRCLE | PSP_CTRL_CROSS | PSP_CTRL_SQUARE | PSP_CTRL_NOTE | PSP_CTRL_HOME | \
	PSP_CTRL_SCREEN | PSP_CTRL_VOLUP | PSP_CTRL_VOLDOWN )

#define XOR_KEY (PSP_CTRL_DISC)

//for go
#define PSP_CTRL_SLIDE 0x20000000

typedef enum {
	KEY_CONFIG_BOOT = 0,
	KEY_CONFIG_OK,
	KEY_CONFIG_CANCEL,
} Key_config;

extern u32 g_key_config[];

inline int detectButtons(u32 buttons, u32 currentButtons, u32 lastButtons, u32 firstDelayTime, u32 delayTime);
int detectButtonsEx(u32 buttons, u32 currentButtons, u32 lastButtons, u32 firstDelayTime, u32 delayTime, u8 *flag, clock_t *time);
void waitPressAnyButtons(void);
void waitButtonsRelease(void);
void getButtons(SceCtrlData *data);
bool isButtonsPushed(unsigned int key);

#endif
