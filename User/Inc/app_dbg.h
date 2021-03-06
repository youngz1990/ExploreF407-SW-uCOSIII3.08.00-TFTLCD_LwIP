/*
  ******************************************************************************
  * @file           : app_dbg.h
  * @brief          : 应用层的调试接口
  ******************************************************************************
  * @功能
  *     1. 使用统一函数接口打印系统状态信息
  * 
  *
  ******************************************************************************
  */
#ifndef __APP_DBG_H
#define __APP_DBG_H

#include "app_lcd.h"

OS_ERR APP_DBGLCDInit(OS_TCB const *p_tcb);
OS_ERR APP_DBGLCDFresh(OS_TCB const *p_tcb);

#endif /* __APP_DBG_H */
