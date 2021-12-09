/*
  ******************************************************************************
  * @file           : system.h
  * @brief          : 不依赖于硬件的基础功能接口
  ******************************************************************************
  * @attention
  *
  * 
  *
  ******************************************************************************
  */

#ifndef __SYS_H
#define __SYS_H

#define DIGIT2STRING_LENGTH_LIMIT            12

void _itoa(int num, char *string, int radix);
void _uitoa(unsigned int num, char *string, int radix);
void _ftoa(float num, char *string, int precision);

#endif /* __SYS_H */
