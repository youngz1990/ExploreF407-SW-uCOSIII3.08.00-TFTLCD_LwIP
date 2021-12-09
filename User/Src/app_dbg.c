/*
******************************************************************************
* @file           : app_dbg.c
* @brief          : 应用层的调试接口实现
******************************************************************************
* @功能
*     
* 
*
******************************************************************************
*/

#include "app_dbg.h"

/* ------------------------------类型定义--------------------------- */


/* ------------------------------全局变量--------------------------- */
BSP_LCDDirTypedef DBG_LCDDir;
/* ------------------------------局部变量--------------------------- */
static BSP_LCDRectTypedef InterfaceTitle_RectText;
static BSP_LCDRectTypedef ErrorCode_RectText; 
static BSP_LCDRectTypedef ErrorCode_RectVal;
static BSP_LCDRectTypedef TaskCount_RectText;
static BSP_LCDRectTypedef TaskCount_RectVal;

/* 表格--调试表格 */
#define LCDTABLE_DBG_LINE_CNT           8
#define LCDTABLE_DBG_ROW_CNT            6

static BSP_LCDTableTypedef TableDBG;

/* 表格--行成员 */
static BSP_LCDTableLineTypedef LineHeader;
static BSP_LCDTableLineTypedef LineTask0;
static BSP_LCDTableLineTypedef LineTask1;
static BSP_LCDTableLineTypedef LineTask2;
static BSP_LCDTableLineTypedef LineTask3;
static BSP_LCDTableLineTypedef LineTask4;
static BSP_LCDTableLineTypedef LineTask5;
static BSP_LCDTableLineTypedef LineTask6;

/* 表格--列成员 */
static BSP_LCDTableRowTypedef RowThreadName;
static BSP_LCDTableRowTypedef RowPrio;
static BSP_LCDTableRowTypedef RowStackUsage;
static BSP_LCDTableRowTypedef RowStackMax;
static BSP_LCDTableRowTypedef RowCPUUsage;
static BSP_LCDTableRowTypedef RowCPUMax;

/* 表格--单元格成员 */
static BSP_LCDTableCellTypedef  TableDBG_Cell[LCDTABLE_DBG_LINE_CNT][LCDTABLE_DBG_ROW_CNT];
static float StackUsageMax[LCDTABLE_DBG_LINE_CNT - 1] = {0};
static float CPUUsageMax[LCDTABLE_DBG_LINE_CNT - 1] = {0};

/* 表格--表头字符串 */
static char Header1[] = "Thread Name";
static char Header2[] = "Prio";
static char Header3[] = "Stack Usage(%)";
static char Header4[] = "Stack Max(%)";
static char Header5[] = "CPU Usage(%)";
static char Header6[] = "CPU Max(%)";

static float StackUsage = 1;
static float CPUUsage = 1;

/* ------------------------------局部函数--------------------------- */

/*
*********************************************************************************************************
*	函    数: APP_DBGLCDInit()
*	说    明: 调试功能初始化
*	形    参: range     LCD屏上显示字符串的区域
            *string   要显示的字符串
            size      显示字体大小
            color     显示字体颜色
*	返    回: 错误码
*********************************************************************************************************
*/
OS_ERR APP_DBGLCDInit(OS_TCB const *p_tcb)
{
  OS_ERR err;
  uint16_t temp = 0;
  char const *p_taskname;
  char string[30] = {0};
  BSP_LCDSpaceTypedef space;
  BSP_LCDCursorTypedef cursor;

  /* 调试界面标题显示区域初始化 */
  space.LCD_LineSpace = BSP_LCD_FONT_HEIGHT_24 / 6;
  space.LCD_WordSpace = 1;

  InterfaceTitle_RectText.LCD_XposStart = 0;
  InterfaceTitle_RectText.LCD_YposStart = BSP_LCD_RESOLUTION_HEIGHT;
  InterfaceTitle_RectText.LCD_XposStop = BSP_LCD_FONT_HEIGHT_24 + space.LCD_LineSpace;
  InterfaceTitle_RectText.LCD_YposStop = 0;
  
  err = App_LCDPend(0);
  if(err != OS_ERR_NONE)
  {
    return err;
  }
  BSP_LCDPrintString(InterfaceTitle_RectText, space, "Debug Interface", BSP_LCD_FONT_HEIGHT_24, BLACK);
  err = App_LCDPost();
  if(err != OS_ERR_NONE)
  {
    return err;
  }

  /* 错误代码显示区域初始化 */
  space.LCD_LineSpace = BSP_LCD_FONT_HEIGHT_16 / 6;
  ErrorCode_RectText.LCD_XposStart = InterfaceTitle_RectText.LCD_XposStop;
  ErrorCode_RectText.LCD_YposStart = BSP_LCD_RESOLUTION_HEIGHT;
  ErrorCode_RectText.LCD_XposStop = ErrorCode_RectText.LCD_XposStart + BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;
  ErrorCode_RectText.LCD_YposStop = 0;

  err = App_LCDPend(0);
  if(err != OS_ERR_NONE)
  {
    return err;
  }
  BSP_LCDPrintString(ErrorCode_RectText, space, "ErrorCode:", BSP_LCD_FONT_HEIGHT_16, BLACK);
  cursor = BSP_LCDGetCursor();
  err = App_LCDPost();
  if(err != OS_ERR_NONE)
  {
    return err;
  }

  ErrorCode_RectVal.LCD_XposStart = cursor.Xpos;
  ErrorCode_RectVal.LCD_YposStart = cursor.Ypos;
  ErrorCode_RectVal.LCD_XposStop = cursor.Xpos + BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;
  ErrorCode_RectVal.LCD_YposStop = 0;

  /* 任务数量显示区域初始化 */
  space.LCD_LineSpace = BSP_LCD_FONT_HEIGHT_16 / 6;
  TaskCount_RectText.LCD_XposStart = ErrorCode_RectText.LCD_XposStop;
  TaskCount_RectText.LCD_YposStart = BSP_LCD_RESOLUTION_HEIGHT;
  TaskCount_RectText.LCD_XposStop = TaskCount_RectText.LCD_XposStart + BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;
  TaskCount_RectText.LCD_YposStop = 0;

  err = App_LCDPend(0);
  if(err != OS_ERR_NONE)
  {
    return err;
  }
  BSP_LCDPrintString(TaskCount_RectText, space, "TaskCount:", BSP_LCD_FONT_HEIGHT_16, BLACK);
  cursor = BSP_LCDGetCursor();
  err = App_LCDPost();
  if(err != OS_ERR_NONE)
  {
    return err;
  }

  TaskCount_RectVal.LCD_XposStart = cursor.Xpos;
  TaskCount_RectVal.LCD_YposStart = cursor.Ypos;
  TaskCount_RectVal.LCD_XposStop = cursor.Xpos + BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;
  TaskCount_RectVal.LCD_YposStop = 0;

  /* 表格对象初始化  */
  TableDBG.TableLinePtr = &LineHeader;
  TableDBG.TableRowPtr = &RowThreadName;
  TableDBG.TableCellPtr = &TableDBG_Cell[0][0];
  TableDBG.TableDir = LCD_DIR_VERTICAL;
  TableDBG.LineCNT = LCDTABLE_DBG_LINE_CNT;
  TableDBG.RowCNT = LCDTABLE_DBG_ROW_CNT;
  TableDBG.XposStart = TaskCount_RectVal.LCD_XposStop + space.LCD_LineSpace;
  TableDBG.YposStart = BSP_LCD_RESOLUTION_HEIGHT -3;

  /* 行对象初始化 */
  LineHeader.PrevPtr = NULL;
  LineHeader.NextPtr = &LineTask0;
  LineHeader.LposStart = TableDBG.XposStart;
  LineHeader.LineNum = 0;
  LineHeader.LineHight = BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;
  
  LineTask0.PrevPtr = &LineHeader;
  LineTask0.NextPtr = &LineTask1;
  LineTask0.LposStart = LineTask0.PrevPtr->LposStart + LineTask0.PrevPtr->LineHight;
  LineTask0.LineNum =1;
  LineTask0.LineHight = BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;

  LineTask1.PrevPtr = &LineTask0;
  LineTask1.NextPtr = &LineTask2;
  LineTask1.LposStart = LineTask1.PrevPtr->LposStart + LineTask1.PrevPtr->LineHight;
  LineTask1.LineNum = 2;
  LineTask1.LineHight = BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;

  LineTask2.PrevPtr = &LineTask1;
  LineTask2.NextPtr = &LineTask3;
  LineTask2.LposStart = LineTask2.PrevPtr->LposStart + LineTask2.PrevPtr->LineHight;
  LineTask2.LineNum = 3;
  LineTask2.LineHight = BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;

  LineTask3.PrevPtr = &LineTask2;
  LineTask3.NextPtr = &LineTask4;
  LineTask3.LposStart = LineTask3.PrevPtr->LposStart + LineTask3.PrevPtr->LineHight;
  LineTask3.LineNum = 4;
  LineTask3.LineHight = BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;

  LineTask4.PrevPtr = &LineTask3;
  LineTask4.NextPtr = &LineTask5;
  LineTask4.LposStart = LineTask4.PrevPtr->LposStart + LineTask4.PrevPtr->LineHight;
  LineTask4.LineNum = 5;
  LineTask4.LineHight = BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;

  LineTask5.PrevPtr = &LineTask4;
  LineTask5.NextPtr = &LineTask6;
  LineTask5.LposStart = LineTask5.PrevPtr->LposStart + LineTask5.PrevPtr->LineHight;
  LineTask5.LineNum = 6;
  LineTask5.LineHight = BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;

  LineTask6.PrevPtr = &LineTask5;
  LineTask6.NextPtr = NULL;
  LineTask6.LposStart = LineTask6.PrevPtr->LposStart + LineTask6.PrevPtr->LineHight;
  LineTask6.LineNum = 7;
  LineTask6.LineHight = BSP_LCD_FONT_HEIGHT_16 + space.LCD_LineSpace;

  /* 列对象初始化  */
  RowThreadName.PrevPtr = NULL;
  RowThreadName.NextPtr = &RowPrio;
  RowThreadName.RposStart = TableDBG.YposStart;
  RowThreadName.RowWidth = 224;
  RowThreadName.RowNum = 0;

  RowPrio.PrevPtr = &RowThreadName;
  RowPrio.NextPtr = &RowStackUsage;
  RowPrio.RposStart = RowPrio.PrevPtr->RposStart - RowPrio.PrevPtr->RowWidth;
  RowPrio.RowWidth = 50;
  RowPrio.RowNum = 1;

  RowStackUsage.PrevPtr = &RowPrio;
  RowStackUsage.NextPtr = &RowStackMax;
  RowStackUsage.RposStart = RowStackUsage.PrevPtr->RposStart - RowStackUsage.PrevPtr->RowWidth;
  RowStackUsage.RowWidth = 150;
  RowStackUsage.RowNum = 2;

  RowStackMax.PrevPtr = &RowStackUsage;
  RowStackMax.NextPtr = &RowCPUUsage;
  RowStackMax.RposStart = RowStackMax.PrevPtr->RposStart - RowStackMax.PrevPtr->RowWidth;
  RowStackMax.RowWidth = 130;
  RowStackMax.RowNum = 3;

  RowCPUUsage.PrevPtr = &RowStackMax;
  RowCPUUsage.NextPtr = &RowCPUMax;
  RowCPUUsage.RposStart = RowCPUUsage.PrevPtr->RposStart - RowCPUUsage.PrevPtr->RowWidth;
  RowCPUUsage.RowWidth = 130;
  RowCPUUsage.RowNum = 4;

  RowCPUMax.PrevPtr = &RowCPUUsage;
  RowCPUMax.NextPtr = NULL;
  RowCPUMax.RposStart = RowCPUMax.PrevPtr->RposStart - RowCPUMax.PrevPtr->RowWidth;
  RowCPUMax.RowWidth = 110;
  RowCPUMax.RowNum = 5;

  /* 表头单元格对象初始化 */
  TableDBG_Cell[0][0].Content = Header1;
  TableDBG_Cell[0][0].Format = STRING;

  TableDBG_Cell[0][1].Content = Header2;
  TableDBG_Cell[0][1].Format = STRING;

  TableDBG_Cell[0][2].Content = Header3;
  TableDBG_Cell[0][2].Format = STRING;

  TableDBG_Cell[0][3].Content = Header4;
  TableDBG_Cell[0][3].Format = STRING;

  TableDBG_Cell[0][4].Content = Header5;
  TableDBG_Cell[0][4].Format = STRING;

  TableDBG_Cell[0][5].Content = Header6;
  TableDBG_Cell[0][5].Format = STRING;

  /* 表格初始化：画表格，确定单元格模式 */
  err = App_LCDPend(0);
  if(err != OS_ERR_NONE)
  {
    return err;
  }
  BSP_LCDTableFrameInit(&TableDBG, &TableDBG_Cell[0][0]);
  err = App_LCDPost();
  if(err != OS_ERR_NONE)
  {
    return err;
  }
  
  /* 填充显示固定字符串的单元格 */
  for(temp = 0; temp < LCDTABLE_DBG_ROW_CNT; temp++)
  {
    err = App_LCDPend(0);
    if(err != OS_ERR_NONE)
    {
      return err;
    }
    BSP_LCDPrintCell(&TableDBG_Cell[0][temp]);
    err = App_LCDPost();
    if(err != OS_ERR_NONE)
    {
      return err;
    }
  }

  temp = 0;

  /* 填充任务名进表格第一列 */
  while(p_tcb != NULL)
  {
    temp++;
    if(temp < LCDTABLE_DBG_LINE_CNT)
    {
      p_taskname = p_tcb->NamePtr;
      TableDBG_Cell[temp][0].Content = (void const *)p_taskname;
      TableDBG_Cell[temp][0].Format = STRING;
      
      err = App_LCDPend(0);
      if(err != OS_ERR_NONE)
      {
        return err;
      }
      BSP_LCDPrintCell(&TableDBG_Cell[temp][0]);
      err = App_LCDPost();
      if(err != OS_ERR_NONE)
      {
        return err;
      }
    }
    p_tcb = p_tcb->DbgNextPtr;
  }

  /* 填充任务数量 */
  _uitoa((unsigned int)temp, string, 10);

  err = App_LCDPend(0);
  if(err != OS_ERR_NONE)
  {
    return err;
  }
  BSP_LCDPrintString(TaskCount_RectVal, space, string, BSP_LCD_FONT_HEIGHT_16, BLACK);
  err = App_LCDPost();
  if(err != OS_ERR_NONE)
  {
    return err;
  }
  return err;
}

/*
*********************************************************************************************************
*	函    数: APP_DBGLCDFresh()
*	说    明: 调试功能初始化
*	形    参: p_tcb   已创建的任务对象OS_TCB链表
*
*	返    回: 错误码
*********************************************************************************************************
*/
OS_ERR APP_DBGLCDFresh(OS_TCB const *p_tcb)
{
  OS_ERR err;
  uint16_t temp0 = 0,temp1 = 0;
  OS_TCB const *ptr = p_tcb;

  for(temp0 = 1; temp0 < LCDTABLE_DBG_ROW_CNT; temp0++)       //逐列填充
  {
    for(temp1 = 1; temp1 < LCDTABLE_DBG_LINE_CNT; temp1++)    //逐行填充
    {
      if(temp0 == 1)     //填充优先级列
      {
        TableDBG_Cell[temp1][temp0].Format = UINT8;
        TableDBG_Cell[temp1][temp0].Content = &(ptr->Prio);
      }
      if(temp0 == 2)     //填充栈使用率
      {
        TableDBG_Cell[temp1][temp0].Format = FLOAT;
        StackUsage = (float)ptr->StkUsed / ptr->StkSize * 100;
        if(StackUsage > StackUsageMax[temp1 - 1])
        {
          StackUsageMax[temp1 - 1] = StackUsage;
        }
        TableDBG_Cell[temp1][temp0].Content = &StackUsage;
      }
      if(temp0 == 3)    //填充最大堆栈使用率
      {
        TableDBG_Cell[temp1][temp0].Format = FLOAT;
        TableDBG_Cell[temp1][temp0].Content = &StackUsageMax[temp1 - 1];
      }
      if(temp0 == 4)    //填充CPU使用率
      {
        TableDBG_Cell[temp1][temp0].Format = FLOAT;
        CPUUsage = ptr->CPUUsage / 100.0;
        TableDBG_Cell[temp1][temp0].Content = &CPUUsage;
        if(CPUUsage > CPUUsageMax[temp1 - 1])
        {
          CPUUsageMax[temp1 - 1] = CPUUsage;
        }
      }
      if(temp0 == 5)    //填充CPU使用率
      {
        TableDBG_Cell[temp1][temp0].Format = FLOAT;
        TableDBG_Cell[temp1][temp0].Content = &CPUUsageMax[temp1 - 1];
      }
      err = App_LCDPend(0);
      if(err != OS_ERR_NONE)
      {
        return err;
      }
      BSP_LCDPrintCell(&TableDBG_Cell[temp1][temp0]);
      err = App_LCDPost();
      if(err != OS_ERR_NONE)
      {
        return err;
      }
      ptr = ptr->DbgNextPtr;
    }
    ptr = p_tcb;
  }
  return err;
}