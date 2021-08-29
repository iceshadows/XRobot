/**
 * @file ctrl_cap.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 超级电容控制线程
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "dev_referee.h"
#include "mod_cap.h"
#include "thd.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */

/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;
static Cap_t cap;

#ifdef MCU_DEBUG_BUILD
CAN_CapOutput_t cap_out;
Referee_ForCap_t referee_cap;
UI_CapUI_t cap_ui;
#else
static CAN_CapOutput_t cap_out;
static Referee_ForCap_t referee_cap;
static UI_CapUI_t cap_ui;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * @brief 控制电容
 *
 * @param argument 未使用
 */
void Thread_CtrlCap(void *argument) {
  RM_UNUSED(argument); /* 未使用argument，消除警告 */

  /* 计算线程运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = pdMS_TO_TICKS(1000 / TASK_FREQ_CTRL_CAP);

  uint32_t previous_wake_time = xTaskGetTickCount();

  while (1) {
    /* 读取裁判系统信息 */
    xQueueReceive(runtime.msgq.referee.cap, &referee_cap, 0);

    /* 一定时间长度内接收不到电容反馈值，使电容离线 */
    if (xQueueReceive(runtime.msgq.can.feedback.cap, &can, 500) != pdPASS) {
      Cap_HandleOffline(&cap, &cap_out, GAME_CHASSIS_MAX_POWER_WO_REF);
    } else {
      vTaskSuspendAll(); /* 锁住RTOS内核防止控制过程中断，造成错误 */
      /* 根据裁判系统数据计算输出功率 */
      Cap_Update(&cap, &(can.cap));
      Cap_Control(&referee_cap, &cap_out);
      xTaskResumeAll();
    }
    /* 将电容输出值发送到CAN */
    xQueueOverwrite(runtime.msgq.can.output.cap, &cap_out);
    /* 将电容状态发送到Chassis */
    xQueueOverwrite(runtime.msgq.cap_info, &(cap));

    Cap_PackUi(&cap, &cap_ui);
    xQueueOverwrite(runtime.msgq.ui.cap, &cap_ui);

    /* 运行结束，等待下一次唤醒 */
    xTaskDelayUntil(&previous_wake_time, delay_tick);
  }
}
