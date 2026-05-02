/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015-2019, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: It is the configure head file for this library.
 * Created on: 2015-07-14
 */

#ifndef EF_CFG_H_
#define EF_CFG_H_

//FLASH 扇区的起始地址
#define ADDR_FLASH_SECTOR_0     ((u32)0x08000000) 	//扇区0起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_1     ((u32)0x08004000) 	//扇区1起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2     ((u32)0x08008000) 	//扇区2起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3     ((u32)0x0800C000) 	//扇区3起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_4     ((u32)0x08010000) 	//扇区4起始地址, 64 Kbytes  
#define ADDR_FLASH_SECTOR_5     ((u32)0x08020000) 	//扇区5起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_6     ((u32)0x08040000) 	//扇区6起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_7     ((u32)0x08060000) 	//扇区7起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_8     ((u32)0x08080000) 	//扇区8起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_9     ((u32)0x080A0000) 	//扇区9起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_10    ((u32)0x080C0000) 	//扇区10起始地址,128 Kbytes  
#define ADDR_FLASH_SECTOR_11    ((u32)0x080E0000) 	//扇区11起始地址,128 Kbytes  

/* using ENV function, default is NG (Next Generation) mode start from V4.0 */
#define EF_USING_ENV

#ifdef EF_USING_ENV
/* Auto update ENV to latest default when current ENV version number is changed. */
/* #define EF_ENV_AUTO_UPDATE */
/**
 * ENV version number defined by user.
 * Please change it when your firmware add a new ENV to default_env_set.
 */
#define EF_ENV_VER_NUM            0/* @note you must define it for a value, such as 0 */
 
/* MCU Endian Configuration, default is Little Endian Order. */
/* #define EF_BIG_ENDIAN  */         

#endif /* EF_USING_ENV */

/* using IAP function */
/* #define EF_USING_IAP */

/* using save log function */
/* #define EF_USING_LOG */

/* The minimum size of flash erasure. May be a flash sector size. */
#define EF_ERASE_MIN_SIZE         (128 * 1024)/* @note you must define it for a value */

/* the flash write granularity, unit: bit
 * only support 1(nor flash)/ 8(stm32f4)/ 32(stm32f1) */
#define EF_WRITE_GRAN             4/* @note you must define it for a value */


/* The size of read_env and continue_ff_addr function used*/
#define EF_READ_BUF_SIZE             32     /* @default 32, Larger numbers can improve first-time speed of alloc_env but require more stack space*/
/*
 *
 * This all Backup Area Flash storage index. All used flash area configure is under here.
 * |----------------------------|   Storage Size
 * | Environment variables area |   ENV area size @see ENV_AREA_SIZE
 * |----------------------------|
 * |      Saved log area        |   Log area size @see LOG_AREA_SIZE
 * |----------------------------|
 * |(IAP)Downloaded application |   IAP already downloaded application, unfixed size
 * |----------------------------|
 *
 * @note all area sizes must be aligned with EF_ERASE_MIN_SIZE
 *
 * The EasyFlash add the NG (Next Generation) mode start from V4.0. All old mode before V4.0, called LEGACY mode.
 *
 * - NG (Next Generation) mode is default mode from V4.0. It's easy to settings, only defined the ENV_AREA_SIZE.
 * - The LEGACY mode has been DEPRECATED. It is NOT RECOMMENDED to continue using.
 *   Beacuse it will use ram to buffer the ENV and spend more flash erase times.
 *   If you want use it please using the V3.X version.
 */

/* backup area start address */
#define EF_START_ADDR             0x080C0000/* @note you must define it for a value */

/* ENV area size. It's at least one empty sector for GC. So it's definition must more then or equal 2 flash sector size. */
#define ENV_AREA_SIZE             (2 * 128 * 1024)/* @note you must define it for a value if you used ENV */

/* saved log area size */
#define LOG_AREA_SIZE             /* @note you must define it for a value if you used log */

/* print debug information of flash */
#define PRINT_DEBUG

#endif /* EF_CFG_H_ */
