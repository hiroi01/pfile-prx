/*
 * file.h
 *
 *  Created on: 2009/12/26
 *      Author: takka
 */

 /* * thanks for iso tool by takka
 *
 * mod by hiroi01
 */
 
#ifndef FILE_H_

#define FILE_H_

#include <stdio.h>

#include <psptypes.h>
#include <pspiofilemgr.h>

#define PSP_O_RDWR_PSP_O_CREAT PSP_O_RDWR|PSP_O_CREAT
#define MAX_PATH_LEN (64)

#define ALIAS_EXTENSION ".pfile.txt"

typedef enum {
  TYPE_TXT = 0,
  TYPE_SHORTCUT,
  TYPE_DIR,
  TYPE_ETC
} file_type;

typedef struct {
  char sort_type;
  char name[MAX_PATH_LEN];
  file_type type;
} dir_t;

typedef struct {
  char name[MAX_PATH_LEN];
  file_type type;
  int iso_flag;
  char iso_name[MAX_PATH_LEN];
  int size;
  int sector;
  void* next_file;
  void* before_file;
  char umd_id[11];
  char e_name[128];
  int umd_size;
  int file_size;
  int num;
} dir_t_2;

#define SECTOR_SIZE (0x800)


extern char dir_type_sort_default[];

/*---------------------------------------------------------------------------
  指定したディレクトリ情報を読取る
---------------------------------------------------------------------------*/
int read_dir(dir_t dir[], int dir_num, const char *path, int dir_only,char *dir_type_sort);
//int read_dir_2(dir_t dir[], const char *path, int read_dir_flag);

//int ms_read(void* buf, const char* path, int pos, int size);

//int ms_write(const void* buf, const char* path, int pos, int size);
//int ms_write_apend(const void* buf, const char* path, int pos, int size);


/*---------------------------------------------------------------------------
  ファイルセクタリード
---------------------------------------------------------------------------*/
//int ms_sector_read(void* buf, const char* path, int sec, int num);

/*---------------------------------------------------------------------------
  ファイルライト
---------------------------------------------------------------------------*/
//int ms_sector_write(const void* buf, const char* path, int sec, int num);

int file_read(void* buf, const char* path, file_type type, int sec, int num);
//int file_write(const void* buf, const char* path, file_type type, int sec, int num);

/*---------------------------------------------------------------------------
---------------------------------------------------------------------------*/
//int set_file_mode(const char* path, int bits);
//int get_file_mode(const char* path);

int up_dir(char *path);

//int get_umd_sector(const char* path, file_type type);

int get_umd_id(char* id, const char* path, file_type type);

//int get_umd_name(char* name, char* e_name, const char* id, int mode);

int read_line(char* str,  SceUID fp, int num);

//int get_ms_free();
int check_ms();
int check_file(const char* path);



/*----------------------------------------------------------------*/
//mod by hiroi01

//nつまり(末尾の)\n(改行) を削除せず、ファイルからそのままの1行を読み込む
int checkMs(void);
int read_line_file(SceUID fp, char* line, int num);
int read_line_file_keepn(SceUID fp, char* line, int num);

/*----------------------------------------------------------------*/

//form ciso.c in iso tool



typedef struct ciso_header
{
    unsigned char magic[4];         /* +00 : 'C','I','S','O'                 */
    unsigned long header_size;      /* +04 : header size (==0x18)            */
    unsigned long long total_bytes; /* +08 : number of original data size    */
    unsigned long block_size;       /* +10 : number of compressed block size */
    unsigned char ver;              /* +14 : version 01                      */
    unsigned char align;            /* +15 : align of index value            */
    unsigned char rsv_06[2];        /* +16 : reserved                        */
}CISO_H;

#define CISO_HEADER_SIZE (0x18)

/*---------------------------------------------------------------------------
  deflateの解凍を行う
    char* o_buff 解凍先
    int o_size   解凍先バッファサイズ
    char* i_buff 入力
    int i_size   入力サイズ

    返値 解凍後のサイズ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int inflate_cso(char* o_buff, int o_size, const char* i_buff, int i_size);

/*---------------------------------------------------------------------------
  CSOから読込む
    char *buf        読込みバッファ
    const char *path パス
    int pos          読込み位置
    int size         読込みサイズ

    返値 実際に読み込んだ長さ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int cso_read(char *buf, const char *path, int pos, int size);

/*---------------------------------------------------------------------------
  CSOから連続で読込む
    char *buf 読込みバッファ
    SceUID fp ファイルポインタ
    int pos   読込み位置
    int size  読込みサイズ

    返値 実際に読み込んだ長さ / エラーの場合は負を返す

    事前にsceIoOpen / 終了後にsceIoCloseが必要
---------------------------------------------------------------------------*/
int cso_read_fp(char *buf, SceUID fp, int pos, int size);

#endif /* FILE_H_ */
