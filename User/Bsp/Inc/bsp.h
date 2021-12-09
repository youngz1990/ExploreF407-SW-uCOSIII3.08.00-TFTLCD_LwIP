/*
  ******************************************************************************
  * @file           : bsp.h
  * @brief          : 硬件驱动接口
  ******************************************************************************
  * @attention
  * 外设：FSMC
  *     bank1 part4，寻址空间0X6C000000~6FFFFFFF
  *     80-series总线接口，地址线A6控制数据/命令读写，16位数据接口
  *     FSMC_BCR1片选控制寄存器
  *         扩展模式使能、写使能、储存器类型SRAM、储存块使能
  *     FSMC_BTR1片选时序寄存器（读操作）
  *         访问模式A、数据保持时间、地址建立时间
  *     FSMC_BWTR1写时序寄存器（写操作）
  *         访问模式A、数据保持时间、地址建立时间
  ******************************************************************************
  */

#ifndef __BSP_H
#define __BSP_H

#include "main.h"
#include "system.h"

/* ------------------------------LCD：颜色定义--------------------------- */
#define WHITE   (uint16_t)0xffff
#define BLACK   (uint16_t)0x0000
#define RED     (uint16_t)0xf800
#define YELLOW  (uint16_t)0xffe0
#define BLUE    (uint16_t)0x001f
#define GRAY    (uint16_t)0X8430

/* -----------------------------LCD：屏幕分辨率-------------------------- */
#define BSP_LCD_RESOLUTION_HEIGHT     800
#define BSP_LCD_RESOLUTION_WIDTH      480

/* ------------------------------LCD：字号参数--------------------------- */
#define BSP_LCD_FONT_HEIGHT_12      12
#define BSP_LCD_FONT_HEIGHT_16      16
#define BSP_LCD_FONT_HEIGHT_24      24
#define BSP_LCD_FONT_WIDTH_12       6
#define BSP_LCD_FONT_WIDTH_16       8
#define BSP_LCD_FONT_WIDTH_24       12

/* -----------------------计算表格内单元格行号列号的宏--------------------- */
#define BSP_LCD_CELL_LINENUM(cell_num, row_cnt)       cell_num / row_cnt        //计算编号为cell_num的单元格在表格中的第几行
#define BSP_LCD_CELL_ROWNUM(cell_num, row_cnt)        cell_num % row_cnt        //计算编号为cell_num的单元格在表格中的第几列

/* -----------------------------BSP:错误类型定义-------------------------- */
typedef enum
{
  BSP_ERROR_NONE = 0,
  BSP_LCD_ERROR_ID = 1001,          //ID错误
  BSP_LCD_ERROR_READ = 1002,        //读出错误
  BSP_LCD_ERROR_FONT = 1003,        //字符错误，系统无该字符
  BSP_LCD_ERROR_RESOLUTION = 1004,  //超出屏幕分辨率
  BSP_LCD_ERROR_RANGE = 1005,       //超出指定的显示范围
  BSP_LCD_ERROR_TABLE = 1006,       //画表错误
  BSP_LCD_ERROR_FORMAT = 1007,      //格式错误
} BSP_ErrorTypedef;

/* ---------------------------------显示元素------------------------------- */
/* LCD显示方向 */
typedef enum
{
  LCD_DIR_CROSSWISE = 0,
  LCD_DIR_VERTICAL,
} BSP_LCDDirTypedef;

/* LCD显示字符的间距和行距 */
typedef struct
{
  uint8_t LCD_WordSpace;
  uint8_t LCD_LineSpace;
} BSP_LCDSpaceTypedef;

/* 矩形 */
typedef struct
{
  uint16_t LCD_XposStart;
  uint16_t LCD_XposStop;
  uint16_t LCD_YposStart;
  uint16_t LCD_YposStop;
} BSP_LCDRectTypedef;

/* 表格元素：行 */
typedef struct bsp_lcd_table_line BSP_LCDTableLineTypedef;
/* 表格元素：列 */
typedef struct bsp_lcd_table_row  BSP_LCDTableRowTypedef;
/* 表格元素：单元格 */
typedef struct bsp_lcd_table_cell BSP_LCDTableCellTypedef;

typedef enum
{
  STRING = 0,
  UINT8,
  UINT16,
  UINT32,
  FLOAT,
  // DOUBLE,
}BSP_LCDCellFormatTypedef;

/* 表格对象--总体属性 */
typedef struct
{
  BSP_LCDTableLineTypedef *TableLinePtr;
  BSP_LCDTableRowTypedef  *TableRowPtr;
  BSP_LCDTableCellTypedef *TableCellPtr;
  BSP_LCDDirTypedef TableDir;
  uint16_t LineCNT;
  uint16_t RowCNT;
  uint16_t XposStart;
  uint16_t YposStart;
}BSP_LCDTableTypedef;

/* 表格对象--行属性 */
struct bsp_lcd_table_line
{
  BSP_LCDTableLineTypedef *PrevPtr;
  BSP_LCDTableLineTypedef *NextPtr;
  uint16_t LposStart;
  uint16_t LineNum;
  uint16_t LineHight;
};

/* 表格对象--列属性 */
struct bsp_lcd_table_row
{
  BSP_LCDTableRowTypedef *PrevPtr;
  BSP_LCDTableRowTypedef *NextPtr;
  uint16_t RposStart;
  uint16_t RowNum;
  uint16_t RowWidth;
};

/* 表格对象--单元格属性 */
struct bsp_lcd_table_cell
{
  BSP_LCDTableCellTypedef   *PrevPtr;
  BSP_LCDTableCellTypedef   *NextPtr;
  BSP_LCDRectTypedef        CellArea;
  uint16_t                  CellNum;
  void const                *Content;
  BSP_LCDCellFormatTypedef  Format;
};


/* --------------------------------LCD部分------------------------------ */
/* LCD屏硬件参数 */
typedef struct
{
  uint16_t LCD_Width;
  uint16_t LCD_Height;
  uint16_t LCD_ID;
  uint16_t LCD_CMD_WriteRAM;
  uint16_t LCD_CMD_SetX;
  uint16_t LCD_CMD_SetY;
  BSP_LCDDirTypedef LCD_DIR;
} BSP_LCDDevTypedef;

/* LCD当前的像素坐标 */
typedef struct
{
  uint16_t Xpos;
  uint16_t Ypos;
} BSP_LCDCursorTypedef;

/* --------------------------------其他部分------------------------------ */

/* ------------------------------接口函数：LCD--------------------------- */
BSP_ErrorTypedef BSP_LCDInit(void);
BSP_ErrorTypedef BSP_LCDPrintString(BSP_LCDRectTypedef range, BSP_LCDSpaceTypedef space, char const *string, uint8_t size, uint16_t color);
void BSP_LCDRectFillPixel(BSP_LCDRectTypedef rect, uint16_t color);
void BSP_LCDDrawRect(BSP_LCDRectTypedef rect, uint16_t color);
BSP_LCDCursorTypedef BSP_LCDGetCursor(void);
BSP_ErrorTypedef   BSP_LCDTableFrameInit(const BSP_LCDTableTypedef *p_table, BSP_LCDTableCellTypedef *p_cell);
BSP_ErrorTypedef   BSP_LCDPrintCell(BSP_LCDTableCellTypedef *p_cell);

#endif /* __BSP_H */
