/*
  ******************************************************************************
  * @file           : app_lcd.h
  * @brief          : lcd显示屏应用层接口
  ******************************************************************************
  * @attention
  *
  * 
  *
  ******************************************************************************
  */

#ifndef __APP_LCD_H
#define __APP_LCD_H

#include "app_cfg.h"
#include "bsp_os.h"
#include "bsp.h"

OS_ERR   App_LCDObjectInit(void);
OS_ERR   App_LCDPend(OS_TICK timeout);
OS_ERR   App_LCDPost(void);

#endif  /* __APP_LCD_H */
