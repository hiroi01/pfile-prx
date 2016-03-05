#ifndef __THREAD_CTRL_H__
#define __THREAD_CTRL_H__

#define THREAD_CTRL_STATE_RESUME 0
#define THREAD_CTRL_STATE_SUSPEND 1

void threadCtrlSetSuspendMode(int mode);
int threadCtrlState();
int threadCtrlInit();
int threadCtrlSuspend();
int threadCtrlResume();

#endif

