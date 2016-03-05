#include "button.h"
#include "main.h"

u32 g_key_config[3] = {
	PSP_CTRL_LTRIGGER|PSP_CTRL_NOTE, //BOOT
	PSP_CTRL_CROSS, //OK
	PSP_CTRL_CIRCLE, //CANCEL
};

static u8 firstFlag;
static clock_t time;

inline int detectButtons(u32 buttons, u32 currentButtons, u32 lastButtons, u32 firstDelayTime, u32 delayTime)
{	
	return detectButtonsEx(buttons, currentButtons, lastButtons, firstDelayTime, delayTime, &firstFlag, &time);
}

int detectButtonsEx(u32 buttons, u32 currentButtons, u32 lastButtons, u32 firstDelayTime, u32 delayTime, u8 *flag, clock_t *time)
{
	if ((currentButtons & lastButtons & buttons) == buttons) {
		if (*flag) {
			if ((sceKernelLibcClock() - *time) >= firstDelayTime) {
				*flag = 0;
				*time = sceKernelLibcClock();
				return 1;
			} else {
				return 0;
			}
		} else {
			if ((sceKernelLibcClock() - *time) >= delayTime) {
				*time = sceKernelLibcClock();
				return 1;
			} else {
				return 0;
			}
		}
	} else if((currentButtons & buttons) == buttons) {
		*time = sceKernelLibcClock();
		*flag = 1;
		return 1;
	}
	
	return 0;
}

void waitPressAnyButtons(void)
{
	SceCtrlData pad;
	while (g_running) {
		getButtons(&pad);
		if (pad.Buttons & CHECK_KEY) {
			break;
		}
		sceKernelDelayThread(100);
	}
}

void waitButtonsRelease(void)
{
	SceCtrlData pad;
	while (g_running) {
		getButtons(&pad);
		if (!(pad.Buttons & CHECK_KEY_2)) {
			break;
		}
		sceKernelDelayThread(100);
	}
}

void getButtons(SceCtrlData *data)
{
	sceCtrlPeekBufferPositive(data, 1);
}

bool isButtonsPushed(u32 key)
{
	SceCtrlData pad;
	getButtons(&pad);
	if ((pad.Buttons & key) == key)
		return true;
	else
		return false;
}

