#include "app_kernal.h"

#include "app_sys.h"
static uint8_t timer_list[TIMER_MAX] = {0};
static Timer *timer_head = NULL;
static uint32_t systick = 0;

//此函数务必被调用，内核系统的时基
void systemTickInc(void)
{
    systick++;
}
uint32_t getSystemTick(void)
{
    return systick;
}

/************************Kernal**********************************/
static uint8_t Create_Timer(uint32_t time, uint8_t timer_id, void (*fun)(void), uint8_t repeat)
{
    Timer *next;
    if (timer_head == NULL)
    {
        timer_head = malloc(sizeof(Timer));
        if (timer_head == NULL)
        {
            return 0;
        }
        timer_head->timer_id = timer_id;
        timer_head->start_tick = getSystemTick();
        timer_head->stop_tick = timer_head->start_tick + time;
        timer_head->repeat = repeat;
        timer_head->repeattime = time;
        timer_head->suspend = 0;
        timer_head->fun = fun;
        timer_head->next = NULL;
        LogPrintf(DEBUG_ALL, "kernal==>Create Head [%d]", timer_id);
        return 1;
    }
    next = timer_head;
    do
    {
        if (next->next == NULL)
        {
            next->next = malloc(sizeof(Timer));
            if (next->next == NULL)
            {
                LogMessage(DEBUG_ALL, "kernal==>Create Fail");
                return 0;
            }
            //LogPrintf(DEBUG_ALL, "Kernal malloc at 0x%X",next->next);
            next = next->next;
            next->fun = fun;
            next->next = NULL;
            next->start_tick = getSystemTick();
            next->stop_tick = next->start_tick + time;
            next->repeat = repeat;
            next->repeattime = time;
            next->suspend = 0;
            next->timer_id = timer_id;
            LogPrintf(DEBUG_ALL, "kernal==>Create Node [%d]", timer_id);
            break;
        }
        else
        {
            next = next->next;
        }
    }
    while (next != NULL);

    return 1;
}

int8_t startTimer(uint32_t time, void (*fun)(void), uint8_t repeat)
{
    int i = 0;
    for (i = 0; i < TIMER_MAX; i++)
    {
        if (timer_list[i] == 0)
        {
            if (Create_Timer(time, i, fun, repeat) == 1)
            {
                timer_list[i] = 1;
                return i;
            }
            else
            {
                break;
            }
        }
    }
    LogMessage(DEBUG_ALL, "kernal==>no space");
    return -1;

}

void stopTimer(uint8_t timer_id)
{
    Timer *next, *pre;
    next = timer_head;
    pre = next;
    while (next != NULL)
    {
        if (next->timer_id == timer_id)
        {
            LogPrintf(DEBUG_ALL, "kernal==>Stop [%d]", timer_id);
            if (next == timer_head)
            {
                next = next->next;
                timer_list[timer_head->timer_id] = 0;
                free(timer_head);
                timer_head = next;
                break;
            }
            timer_list[next->timer_id] = 0;
            pre->next = next->next;
            free(next);
            next = pre;
            break;
        }
        pre = next;
        next = next->next;
    }
}

void stopTimerRepeat(uint8_t timer_id)
{
    Timer *next;
    next = timer_head;
    while (next != NULL)
    {
        if (next->timer_id == timer_id)
        {
            next->repeat = 0;
        }
        next = next->next;
    }
}
void kernalRun(void)
{
    Timer *next, *pre, *cur;
    next = timer_head;
    pre = next;
    systemTickInc();
    while (next != NULL)
    {
        if (next->suspend == 0 && next->stop_tick <= getSystemTick())
        {
            next->fun();
            if (next->repeat == 0)
            {
                if (next == timer_head)
                {
                    timer_list[next->timer_id] = 0;
                    LogPrintf(DEBUG_ALL, "kernal==>Destory [%d]", next->timer_id);
                    next = next->next;
                    free(timer_head);
                    timer_head = next;
                    continue;
                }
                cur = next;
                timer_list[cur->timer_id] = 0;
                LogPrintf(DEBUG_ALL, "kernal==>Destory [%d]", cur->timer_id);
                pre->next = cur->next;
                next = pre;
                free(cur);
            }
            else
            {
                next->stop_tick = getSystemTick() + next->repeattime;
            }
        }
        pre = next;
        next = next->next;
    }
}

//挂起任务
void systemTaskSuspend(uint8_t taskid)
{
    Timer *next;
    next = timer_head;
    while (next != NULL)
    {
        if (next->timer_id == taskid)
        {
            next->suspend = 1;
            LogPrintf(DEBUG_ALL, "kernal==>Suspend [%d]", next->timer_id);
            return ;
        }
        next = next->next;
    }
}
//恢复任务
void systemTaskResume(uint8_t taskid)
{
    Timer *next;
    char debug[50];
    next = timer_head;
    while (next != NULL)
    {
        if (next->timer_id == taskid)
        {
            next->suspend = 0;
            next->stop_tick = 0; //立即执行该任务
            LogPrintf(DEBUG_ALL, "kernal==>Resume [%d]", next->timer_id);
            return;
        }
        next = next->next;
    }
}

int8_t createSystemTask(void(*fun)(void), uint32_t cycletime)
{
    return startTimer(cycletime, fun, 1);
}


