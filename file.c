/*
 * file.c
 *
 *  Created on: 2009/12/26
 *      Author: takka
 */


/* 
 * thanks for iso tool by takka
 *
 * mod by hiroi01
 */
 

//#include <zlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef EMU
#include <pspkernel.h>
#include <pspmscm.h>
#endif

#include "button.h"
#include "file.h"

#include "log.h"

#define FIO_CST_SIZE    0x0004

//--------------------------------------------------------


typedef enum {
  ERR_OPEN          = -1,
  ERR_READ          = -2,
  ERR_WRITE         = -3,
  ERR_SEEK          = -4,
  ERR_CLOSE         = -5,

  ERR_DECRYPT       = -6,
  ERR_NOT_CRYPT     = -7,

  ERR_DEFLATE       = -8,
  ERR_DEFLATE_SIZE  = -9,
  ERR_INFLATE       = -10,

  ERR_INIT          = -11,

  ERR_PRX           = -12,

  ERR_NOT_FOUND     = -13,
  ERR_SIZE_OVER     = -14,

  ERR_CHG_STAT      = -15,

  ERR_NO_UMD        = -16,

  ERR_RENAME        = -17,

  ERR_NO_MEMORY     = -18,

} err_msg_num;

#define YES                 (1)
#define NO                  (0)

#define DONE                (0)
#define CANCEL              (-1)
//--------------------------------------------------------




int compare_dir_int(const void* c1, const void* c2);
int compare_dir_str(const void* c1, const void* c2);
int compare_dir_dir(const void* c1, const void* c2);





// ソート時の優先順位
char dir_type_sort_default[] = {
    'b', // TYPE_TXT
    'b', // TYPE_SHORTCUT
    'b', // TYPE_DIR
    'b', // TYPE_ETC
};

int compare_dir_str(const void* c1, const void* c2)
{
  return strcasecmp(&(((dir_t *)c1)->sort_type), &(((dir_t *)c2)->sort_type));
}

/*---------------------------------------------------------------------------
  ディレクトリ読取り
  dir_t dir[]      : dir_t配列のポインタ
  const char *path : パス

  return int       : ファイル数, dir[0].numにも保存される
---------------------------------------------------------------------------*/
int read_dir(dir_t dir[], int dir_num, const char *path, int dir_only,char *dir_type_sort)
{
    SceUID dp;
    int num;
    int file_num = 0;
    
    if( dir_type_sort == NULL )
        dir_type_sort = (char *)dir_type_sort_default;
    
    //  checkMs();
    //  int ret  = check_ms();
    
    dbgprintf("dopen(%s)\n", path);
    dp = sceIoDopen(path);
    if(dp >= 0)
    {
		SceIoDirent entry;
        memset(&entry, 0, sizeof(entry));
        
        while((sceIoDread(dp, &entry) > 0) && dir_num-- > 0)
        {
            num = strlen(entry.d_name);
            
            strcpy(dir[file_num].name, entry.d_name);
            switch(entry.d_stat.st_mode & FIO_S_IFMT)
            {
                case FIO_S_IFREG:
                    if(dir_only == 0)
                    {
                        if(strncasecmp(&entry.d_name[num -  sizeof(ALIAS_EXTENSION)-1],  ALIAS_EXTENSION,  sizeof(ALIAS_EXTENSION)-1) == 0)
                        {
                            dir[file_num].type = TYPE_SHORTCUT;
                            dir[file_num].sort_type = dir_type_sort[TYPE_SHORTCUT];
                            file_num++;                            
                        }
                        else if(strncasecmp(&entry.d_name[num - 4], ".txt", 4) == 0)
                        {
                            dir[file_num].type = TYPE_TXT;
                            dir[file_num].sort_type = dir_type_sort[TYPE_TXT];
                            file_num++;
                        }
                        else
                        {
                            dir[file_num].type = TYPE_ETC;
                            dir[file_num].sort_type = dir_type_sort[TYPE_ETC];
                            file_num++;
                        }
                    }
                    break;
                    
                case FIO_S_IFDIR:
                    if((strcmp(&entry.d_name[0], ".") == 0) || (strcmp(&entry.d_name[0], "..") == 0)){
                        dir_num++;
                    }
                    else
                    {
                        dir[file_num].type = TYPE_DIR;
                        dir[file_num].sort_type = dir_type_sort[TYPE_DIR];
                        file_num++;
                    }
                    break;
            }
        }
        sceIoDclose(dp);
        qsort(dir, file_num, sizeof(dir_t), compare_dir_str);
    }
    
    
    //  dir[0].num = file_num;
    
    return file_num;
}

/*---------------------------------------------------------------------------
  ディレクトリ読取り
  dir_t dir[]      : dir_t配列のポインタ
  const char *path : パス

  return int       : ファイル数, dir[0].numにも保存される
---------------------------------------------------------------------------*/
/*
int read_dir_2(dir_t dir[], const char *path, int read_dir_flag)
{
  SceUID dp;
  SceIoDirent entry;
  int num;
  int file_num = 0;
  int ret;

  ret = check_ms();

  dp = sceIoDopen(path);
  if(dp >= 0)
  {
    memset(&entry, 0, sizeof(entry));

    while((sceIoDread(dp, &entry) > 0))
    {
      num = strlen(entry.d_name);

      strcpy(dir[file_num].name, entry.d_name);
      switch(entry.d_stat.st_mode & FIO_S_IFMT)
      {
        case FIO_S_IFREG:
          dir[file_num].type = TYPE_ETC;
          dir[file_num].sort_type = dir_type_sort[TYPE_ETC];
          file_num++;
          break;

        case FIO_S_IFDIR:
          if(read_dir_flag == 1)
          {
            if((strcmp(&entry.d_name[0], ".") != 0) && (strcmp(&entry.d_name[0], "..") != 0))
            {
              dir[file_num].type = TYPE_DIR;
              dir[file_num].sort_type = dir_type_sort[TYPE_DIR];
              file_num++;
            }
          }
          break;
      }
    }
    sceIoDclose(dp);
  }

  dir[0].num = file_num;

  // ファイル名でソート
//  qsort(&dir[0], file_num - 1, sizeof(dir_t), compare_dir_str);

  return file_num;
}
*/

/*---------------------------------------------------------------------------
  MSのリード
  void* buf        : 読取りバッファ
  const char* path : パス
  int pos          : 読込み開始場所
  int size         : 読込みサイズ, 0を指定すると全てを読込む

  return int       : 読込みサイズ, エラーの場合は ERR_OPEN/ERR_READ を返す
---------------------------------------------------------------------------*/
/*
int ms_read(void* buf, const char* path, int pos, int size)
{
  SceUID fp;
  SceIoStat stat;
  int ret = ERR_OPEN;

  if(size == 0)
  {
    pos = 0;
    sceIoGetstat(path, &stat);
    size = stat.st_size;
  }

  fp = sceIoOpen(path, PSP_O_RDONLY, 0777);
  if(fp > 0)
  {
    sceIoLseek32(fp, pos, PSP_SEEK_SET);
    ret = sceIoRead(fp, buf, size);
    sceIoClose(fp);
    if(ret < 0)
      ret = ERR_READ;
  }
  return ret;
}
*/

/*---------------------------------------------------------------------------
  MSへのライト
  const void* buf  : 書込みバッファ
  const char* path : パス
  int pos          : 書込み開始場所
  int size         : 書込みサイズ

  return int       : 書込んだサイズ, エラーの場合は ERR_OPEN/ERR_WRITE を返す
---------------------------------------------------------------------------*/
/*
int ms_write(const void* buf, const char* path, int pos, int size)
{
  SceUID fp;
  int ret = ERR_OPEN;

  fp = sceIoOpen(path, PSP_O_WRONLY|PSP_O_CREAT, 0777);
  if(fp > 0)
  {
    sceIoLseek32(fp, pos, PSP_SEEK_SET);
    ret = sceIoWrite(fp, buf, size);
    sceIoClose(fp);
    if(ret < 0)
      ret = ERR_WRITE;
  }
  return ret;
}

int ms_write_apend(const void* buf, const char* path, int pos, int size)
{
  SceUID fp;
  int ret = ERR_OPEN;

  fp = sceIoOpen(path, PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
  if(fp > 0)
  {
    sceIoLseek32(fp, pos, PSP_SEEK_SET);
    ret = sceIoWrite(fp, buf, size);
    sceIoClose(fp);
    if(ret < 0)
      ret = ERR_WRITE;
  }
  return ret;
}
*/
/*---------------------------------------------------------------------------
  ファイルリード
---------------------------------------------------------------------------*/
/*
int file_read(void* buf, const char* path, file_type type, int pos, int size)
{
  int ret = ERR_OPEN;

  switch(type)
  {
    case TYPE_ISO:
    case TYPE_SYS:
      ret = ms_read(buf, path, pos, size);
      break;

    case TYPE_CSO:
      ret = cso_read(buf, path, pos, size);
      break;

    case TYPE_UMD:
      ret = umd_read(buf, path, pos, size);
      break;

    default:
      break;
  }
  return ret;
}
*/

/*---------------------------------------------------------------------------
  ファイルライト
---------------------------------------------------------------------------*/
/*
int file_write(const void* buf, const char* path, file_type type, int pos, int size)
{
  u32 ret = ERR_OPEN;

  switch(type)
  {
    case TYPE_ISO:
      ret = ms_write(buf, path, pos, size);
      break;

    case TYPE_CSO:
      ret = cso_write(buf, path, pos, size, 9);
      break;

    case TYPE_UMD:
      break;

    default:
      break;
  }
  return ret;
}

// FIO_S_IWUSR | FIO_S_IWGRP | FIO_S_IWOTH
int set_file_mode(const char* path, int bits)
{
  SceIoStat stat;
  int ret;

  ret = sceIoGetstat(path, &stat);

  if(ret >= 0)
  {
    stat.st_mode |= (bits);
    ret = sceIoChstat(path, &stat, (FIO_S_IRWXU | FIO_S_IRWXG | FIO_S_IRWXO));
  }
  if(ret < 0)
    ret = ERR_CHG_STAT;

  return ret;
}
*/
/*---------------------------------------------------------------------------
---------------------------------------------------------------------------*/
int up_dir(char *path)
{
  int loop;
  int ret = ERR_OPEN;

  loop = strlen(path) - 2;

  while(path[loop--] != '/');

  if(path[loop - 1] != ':')
  {
    path[loop + 2] = '\0';
    ret = 0;
  }

  return ret;
}



int read_line(char* str,  SceUID fp, int num)
{
  char buf;
  int len = 0;
  int ret;

  do{
    ret = sceIoRead(fp, &buf, 1);
    if(ret == 1)
    {
      if(buf == '\n')
      {
        str[len] = '\0';
        len++;
        break;
      }
      if(buf != '\r')
      {
        str[len] = buf;
        len++;
      }
    }
  }while((ret > 0) && (len < num));

  return len;
}



//from umd dumper by takka
int read_line_file(SceUID fp, char* line, int num)
{
  char buff[num];
  char* end;
  int len;
  int tmp;

  tmp = 0;
  len = sceIoRead(fp, buff, num);
  //  on error
  if (len == 0) {
    return -1;
  }

  end = strchr(buff, '\n');

  // cannot found '\n'
  if (end == NULL) {
    buff[num - 1] = '\0';
    strcpy(line, buff);
    return len;
  }

  end[0] = '\0';
  if((end != buff) && (end[-1] == '\r'))
  {
    end[-1] = '\0';
    tmp = -1;
  }

  strcpy(line, buff);
  sceIoLseek(fp, - len + (end - buff) + 1, SEEK_CUR);
  return end - buff + tmp;
}

int check_file(const char* path)
{
  SceIoStat stat;
  return sceIoGetstat(path, &stat);
}


