#include <pspkernel.h>
#include <systemctrl.h>

#include <string.h>

#include "threadctrl.h"

/* _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/ */

#define MAX_THREAD 64

#define IO_MEM_STICK_STATUS *((volatile int*)(0xBD200038))

/* _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/ */

static int first_thid[MAX_THREAD];
static int first_count;

static int current_thid[MAX_THREAD];
static int current_count = -1;

static unsigned char use_safely_suspend = 1;

/* _/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/ */

int threadCtrlState()
{
	return (current_count < 0) ? THREAD_CTRL_STATE_RESUME : THREAD_CTRL_STATE_SUSPEND;
}

void threadCtrlSetSuspendMode(int mode)
{
	use_safely_suspend = (mode == 0)?1:0;
}

int threadCtrlInit()
{
	return sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, first_thid, MAX_THREAD, &first_count);
}

int threadCtrlSuspend()
{
	if (threadCtrlState() == THREAD_CTRL_STATE_SUSPEND) {
		return 1;
	}
	
	int i, n;
	SceUID this_thid;
	SceKernelThreadInfo thinfo;
	
	this_thid = sceKernelGetThreadId();
	
	sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, current_thid, MAX_THREAD, &current_count);
	
	for (i = 0;i < current_count;i++) {
		memset(&thinfo, 0, sizeof(SceKernelThreadInfo));
		thinfo.size = sizeof(SceKernelThreadInfo);
		sceKernelReferThreadStatus(current_thid[i], &thinfo);
		
		if (thinfo.status & PSP_THREAD_SUSPEND || current_thid[i] == this_thid) {
			current_thid[i] = -1;
			continue;
		}
		for (n = 0;n < first_count;n++) {
			if (current_thid[i] == first_thid[n]) {
				current_thid[i] = -1;
				break;
			}
		}
	}
	
	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	/*      I got a hint from taba's JPCheat, thanks!        */
	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	if (use_safely_suspend) {
		int count;
		for (count = 0;count < 1000;count++) {
			if ((IO_MEM_STICK_STATUS & 0x2000) == 0) {
				count = 0;
			}
			sceKernelDelayThread(1);
		}
	}
	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	
	for (i = 0;i < current_count;i++) {
		if (current_thid[i] >= 0) {
			sceKernelSuspendThread(current_thid[i]);
		}
	}
	
	return 0;
}

int threadCtrlResume()
{
	if (threadCtrlState() == THREAD_CTRL_STATE_RESUME) {
		return 1;
	}
	
	int i;
	for (i = 0;i < current_count;i++) {
		if (current_thid[i] >= 0)
			sceKernelResumeThread(current_thid[i]);
	}
	
	current_count = -1;
	
	return 0;
}


