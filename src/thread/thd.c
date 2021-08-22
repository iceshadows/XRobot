/**
 * @file user_task.c
 * @author Qu Shen (503578404@qq.com)
 * @brief
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 保存任务属性：堆栈大小、优先级等
 * 生成任务时使用。
 *
 * @note 所有直接处理物理设备的任务（CAN、DR16等）优先级应该最高
 * 设备之间有心计可以不一样，但是需要极其小心
 * 运行模块（module）任务的优先级应该低于物理设备任务优先级
 * 其他辅助运行的非核心功能应该更低
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "thd.h"

#include "bsp_flash.h"
#include "comp_cmd.h"
#include "dev_ai.h"
#include "dev_bmi088.h"
#include "dev_can.h"
#include "dev_ist8310.h"
#include "dev_referee.h"
#include "mod_cap.h"
#include "mod_chassis.h"
#include "mod_gimbal.h"
#include "mod_launcher.h"

void Task_Ai(void *argument);
void Task_AttiEsti(void *argument);
void Task_Can(void *argument);
void Task_CLI(void *argument);
void Task_Cmd(void *argument);
void Task_CtrlCap(void *argument);
void Task_CtrlChassis(void *argument);
void Task_CtrlGimbal(void *argument);
void Task_CtrlLauncher(void *argument);
void Task_Info(void *argument);
void Task_Monitor(void *argument);
void Task_RC(void *argument);
void Task_Referee(void *argument);

/* 机器人运行时的数据 */
Runtime_t runtime;

/**
 * @brief 初始化
 *
 * @param argument 未使用
 */
void Task_Init(void *argument) {
  UNUSED(argument); /* 未使用argument，消除警告 */

  Config_Get(&runtime.cfg); /* 获取机器人配置 */

  osKernelLock();
  /* 创建任务, 优先级随数字增大而增大 */
  xTaskCreate(Task_AttiEsti, "AttiEsti", 256, NULL, 5,
              &runtime.thread.atti_esti);
  xTaskCreate(Task_CLI, "CLI", 256, NULL, 3, &runtime.thread.cli);
  xTaskCreate(Task_Cmd, "Cmd", 128, NULL, 4, &runtime.thread.cmd);
  xTaskCreate(Task_CtrlCap, "CtrlCap", 128, NULL, 3, &runtime.thread.ctrl_cap);
  xTaskCreate(Task_CtrlChassis, "CtrlChassis", 256, NULL, 3,
              &runtime.thread.ctrl_chassis);
  xTaskCreate(Task_CtrlGimbal, "CtrlGimbal", 256, NULL, 3,
              &runtime.thread.ctrl_gimbal);
  xTaskCreate(Task_CtrlLauncher, "CtrlLauncher", 256, NULL, 3,
              &runtime.thread.ctrl_launcher);
  xTaskCreate(Task_Info, "Info", 128, NULL, 2, &runtime.thread.info);
  xTaskCreate(Task_Monitor, "Monitor", 128, NULL, 2, &runtime.thread.monitor);
  xTaskCreate(Task_Can, "Can", 128, NULL, 5, &runtime.thread.can);
  xTaskCreate(Task_Referee, "Referee", 512, NULL, 5, &runtime.thread.referee);
  xTaskCreate(Task_Ai, "Ai", 128, NULL, 5, &runtime.thread.ai);
  xTaskCreate(Task_RC, "RC", 128, NULL, 5, &runtime.thread.rc);

  /* 创建消息队列 */
  /* motor */
  runtime.msgq.can.feedback.chassis = xQueueCreate(1u, sizeof(CAN_t));
  runtime.msgq.can.feedback.gimbal = xQueueCreate(1u, sizeof(CAN_t));
  runtime.msgq.can.feedback.launcher = xQueueCreate(1u, sizeof(CAN_t));
  runtime.msgq.can.feedback.cap = xQueueCreate(1u, sizeof(CAN_t));
  runtime.msgq.can.output.chassis =
      xQueueCreate(1u, sizeof(CAN_ChassisOutput_t));
  runtime.msgq.can.output.gimbal = xQueueCreate(1u, sizeof(CAN_GimbalOutput_t));
  runtime.msgq.can.output.launcher =
      xQueueCreate(1u, sizeof(CAN_LauncherOutput_t));
  runtime.msgq.can.output.cap = xQueueCreate(1u, sizeof(CAN_CapOutput_t));

  /* cmd */
  runtime.msgq.cmd.chassis = xQueueCreate(1u, sizeof(CMD_ChassisCmd_t));
  runtime.msgq.cmd.gimbal = xQueueCreate(1u, sizeof(CMD_GimbalCmd_t));
  runtime.msgq.cmd.launcher = xQueueCreate(1u, sizeof(CMD_LauncherCmd_t));
  runtime.msgq.cmd.ai = xQueueCreate(1u, sizeof(Game_AI_Mode_t));

  /* atti_esti */
  runtime.msgq.cmd.src.rc = xQueueCreate(1u, sizeof(CMD_RC_t));
  runtime.msgq.cmd.src.host = xQueueCreate(1u, sizeof(CMD_Host_t));

  runtime.msgq.gimbal.accl = xQueueCreate(1u, sizeof(Vector3_t));
  runtime.msgq.gimbal.eulr_imu = xQueueCreate(1u, sizeof(AHRS_Eulr_t));
  runtime.msgq.gimbal.gyro = xQueueCreate(1u, sizeof(Vector3_t));

  runtime.msgq.cap_info = xQueueCreate(1u, sizeof(Cap_t));

  /* AI */
  runtime.msgq.ai.quat = xQueueCreate(1u, sizeof(AHRS_Quaternion_t));

  /* 裁判系统 */
  runtime.msgq.referee.ai = xQueueCreate(1u, sizeof(Referee_ForAI_t));
  runtime.msgq.referee.chassis = xQueueCreate(1u, sizeof(Referee_ForChassis_t));
  runtime.msgq.referee.cap = xQueueCreate(1u, sizeof(Referee_ForCap_t));
  runtime.msgq.referee.launcher =
      xQueueCreate(1u, sizeof(Referee_ForLauncher_t));

  /* UI */
  runtime.msgq.ui.chassis = xQueueCreate(1u, sizeof(UI_ChassisUI_t));
  runtime.msgq.ui.cap = xQueueCreate(1u, sizeof(UI_CapUI_t));
  runtime.msgq.ui.gimbal = xQueueCreate(1u, sizeof(UI_GimbalUI_t));
  runtime.msgq.ui.launcher = xQueueCreate(1u, sizeof(UI_LauncherUI_t));
  runtime.msgq.ui.cmd = xQueueCreate(1u, sizeof(CMD_UI_t));
  runtime.msgq.ui.ai = xQueueCreate(1u, sizeof(AI_UI_t));

  osKernelUnlock();
  osThreadTerminate(osThreadGetId()); /* 结束自身 */
}
