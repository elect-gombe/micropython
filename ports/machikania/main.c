#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/mperrno.h"
#include "lib_video_megalopa.h"
#include "lib/utils/pyexec.h"
#include "lib/mp-readline/readline.h"
#include "ps2keyboard.h"
#include <xc.h>
#include "py/gc.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include <ctype.h>



#pragma config FSRSSEL = PRIORITY_7
#pragma config PMDL1WAY = OFF
#pragma config IOL1WAY = OFF
#pragma config FPLLIDIV = DIV_3
#pragma config FPLLMUL = MUL_20
#pragma config FPLLODIV = DIV_1
#pragma config FNOSC = PRIPLL
#pragma config FSOSCEN = OFF
#pragma config IESO = OFF
#pragma config POSCMOD = XT
#pragma config OSCIOFNC = OFF
#pragma config FPBDIV = DIV_1
#pragma config FCKSM = CSDCMD
#pragma config FWDTEN = OFF
#pragma config DEBUG = OFF
#pragma config PWP = OFF
#pragma config BWP = OFF
#pragma config CP = OFF


fs_user_mount_t fs_user_mount_flash;

int mp_hal_ticks_ms(void){
  return drawcount*1000/60;
}

void do_str(const char *src, mp_parse_input_kind_t input_kind) {
  nlr_buf_t nlr;
  if (nlr_push(&nlr) == 0) {
    mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
    qstr source_name = lex->source_name;
    mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
    mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
    mp_call_function_0(module_fun);
    nlr_pop();
  } else {
    // uncaught exception
    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
  }
}

void wait60thsec(int n){
  do{
    while(!drawing);
    while(drawing);
  }while(--n);
}

static char heap[0x1A800];
int main(int argc, char **argv) {
  void *stack;
  MP_STATE_THREAD(stack_top) = (char*)&stack;
  /* ポートの初期設定 */
  CNPUB = 0xFFFF; // PORTB全てプルアップ(I/O)
  TRISB = 0xFFFF; // PORTB全て入力
  CNPUC = 0x4000; // PORTC14プルアップ(U1RX)
  TRISC = 0x4000; // PORTC14以外は出力
  TRISD = KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT;// ボタン接続ポート入力設定
  CNPUE = 0x00E0; // PORTE5-7プルアップ(I/O)
  TRISE = 0x00E0; // PORTE0-4出力5-7入力
  CNPUF = 0x0004; // PORTF2プルアップ(SDI1)
  TRISF = 0x0004; // PORTF2以外は出力
  TRISG = 0x0080; // PORTG7以外は出力

  ANSELB = 0x0000; // 全てデジタル
  ANSELD = 0x0000; // 全てデジタル
  ANSELE = 0x0000; // 全てデジタル
  ANSELG = 0x0000; // 全てデジタル
  CNPUDSET=KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT;// プルアップ設定
  CNPUFSET=0x0004; // PORTF2 (SDI1) プルアップ
  ODCF = 0x0003;	//RF0,RF1はオープンドレイン
  ODCG = 0x000c;//RG2,RG3はオープンドレイン
  LATGSET = 0x000c;

  // 周辺機能ピン割り当て
  SDI2R = 1; //RPG7にSDI2を割り当て
  RPG8R = 6; //RPG8にSDO2を割り当て

  init_composite();
  set_videomode(VMODE_WIDETEXT,NULL);
  
  // init the vfs object

  if(ps2init()){
    printstr("err while init ps2 keyboard\n");
  }
  /* printstr("micropython test\n"); */
  gc_init(heap, heap + sizeof(heap));

  /* printstr("gc\n"); */
  /* wait60thsec(60); */
  mp_init();
  /* printstr("mp\n"); */
  /* wait60thsec(60); */
  /* do_str("print('hello world!', list(x+1 for x in range(10)), end='eol\\n')", MP_PARSE_SINGLE_INPUT );*/
  /* wait60thsec(60); */
  /* do_str("for i in range(10):\n  print(i)", MP_PARSE_FILE_INPUT); */
  /* printf("exit success\n"); */
  readline_init0();

    fs_user_mount_t *vfs_fat = &fs_user_mount_flash;
  vfs_fat->flags = 0;
  pyb_flash_init_vfs(vfs_fat);

  // try to mount the flash
  FRESULT res = f_mount(&vfs_fat->fatfs);
  if(res){
    printstr("initfaild\n");
    printnum(res);
    wait60thsec(60);
  }
  FILINFO fno;

  res = f_stat(&vfs_fat->fatfs, "/test.py", &fno);
  if(res){
    printstr("file not found\n");
    printnum(res);
    wait60thsec(60);
  }
  

  // mount the flash device (there should be no other devices mounted at this point)
  // we allocate this structure on the heap because vfs->next is a root pointer
  mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
  if (vfs == NULL) {
    printstr("vfs failed");
    while(1);
  }
  vfs->str = "/flash";
  vfs->len = 6;
  vfs->obj = MP_OBJ_FROM_PTR(vfs_fat);
  vfs->next = NULL;
  MP_STATE_VM(vfs_mount_table) = vfs;

  // The current directory is used as the boot up directory.
  // It is set to the internal flash filesystem by default.
  MP_STATE_PORT(vfs_cur) = vfs;

  mp_obj_list_init(mp_sys_path, 0);
  mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));

  // REPL loop
  for (;;) {
    if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
      if (pyexec_raw_repl() != 0) {
	break;
      }
    } else {
      if (pyexec_friendly_repl() != 0) {
	break;
      }
    }
  }

  while(1);
  mp_deinit();
  return 0;
}

#define BLINKTIME 20
unsigned char blinkchar,blinkcolor;
int blinktimer;
int insertmode=1; //挿入モード：1、上書きモード：0

#define CURSORCHAR 0x87
#define CURSORCHAR2 0x80
#define CURSORCOLOR 7

void getcursorchar(){
// カーソル点滅用に元の文字コードを退避
	blinkchar=*cursor;
	blinkcolor=*(cursor+attroffset);
}
void resetcursorchar(){
// カーソルを元の文字に戻す
	*cursor=blinkchar;
	*(cursor+attroffset)=blinkcolor;
}
void blinkcursorchar(){
// 定期的に呼び出すことでカーソルを点滅表示させる
// BLINKTIMEで点滅間隔を設定
// 事前にgetcursorchar()を呼び出しておく
	blinktimer++;
	if(blinktimer>=BLINKTIME*2) blinktimer=0;
	if(blinktimer<BLINKTIME){
		if(insertmode) *cursor=CURSORCHAR;
		else *cursor=CURSORCHAR2;
		*(cursor+attroffset)=CURSORCOLOR;
	}
	else{
		*cursor=blinkchar;
		*(cursor+attroffset)=blinkcolor;
	}
}



int mp_hal_stdin_rx_chr(void) {
  static char pc[6];
  int i;
  char c = 0;
  if(pc[0]){
    char r;
    r = pc[0];
    for(i=0;i<5;i++){
      pc[i]=pc[i+1];
    }
    c = r;
  }
  getcursorchar();
  for (;!c;) {
    wait60thsec(1);
    blinkcursorchar();
    c = (char)ps2readkey();
    if(vkey& 0x0200){//Ctrl
      if('A'<=(vkey&0xFF)&&(vkey&0xFF)<='Z'){
	c= (vkey&0xFF)-'A'+1;
	break;
      }
    }
    if(!c&&vkey&0xFF){
      switch(vkey&0xFF){
      case VK_DELETE:
	//	snprintf(pc,sizeof(pc),"[3~");
	c='\x04';
	break;
      case VK_RETURN:
      case VK_SEPARATOR:
	c =  '\r';
	break;
      case VK_BACK:
	c =  '\b';
	break;
      case VK_TAB:
	c =  '\t';
	break;
      case VK_UP:
	snprintf(pc,sizeof(pc),"[A");
	c =  '\033';
	break;
      case VK_DOWN:
	snprintf(pc,sizeof(pc),"[B");
	c =  '\033';
	break;
      case VK_RIGHT:
	snprintf(pc,sizeof(pc),"[C");
	c =  '\033';
	break;
      case VK_LEFT:
	snprintf(pc,sizeof(pc),"[D");
	c =  '\033';
	break;
      }
    }
  }
  resetcursorchar();
  return c;
}

void *gc_savereg(void **p);

void gc_collect(void) {
  // TODO possibly need to trace registers
  void *s,*p[30];
  s = gc_savereg(p);
  /* mp_printf(&mp_plat_print,"gc info %p~%p\n",s,MP_STATE_THREAD(stack_top)); */
  gc_collect_start();
  // Node: stack is ascending
  gc_collect_root(s, ((mp_uint_t)MP_STATE_THREAD(stack_top)-(uint32_t)s) / sizeof(mp_uint_t));
  gc_collect_end();
}

void mp_hal_stdout_tx_str(const char *str) {
  printstr(str);
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
  char m[3];
  m[1] = '\0';
    for (; len > 0; --len) {
      m[0] = *str++;
      printstr(m);
    }
}

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
  mp_hal_stdout_tx_strn(str,len);
}


void nlr_jump_fail(void *val) {
  printstr("jump failed\n");
  printnum((int)val);
  while (1);
}

void NORETURN __fatal_error(const char *msg) {
  printstr("err");
  printstr(msg);
  while (1);
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
  printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
  __fatal_error("Assertion failed");
}
#endif

/*
  int _lseek() {return 0;}
  int _read() {return 0;}
  int _write() {return 0;}
  int _close() {return 0;}
  void _exit(int x) {for(;;){}}
  int _sbrk() {return 0;}
  int _kill() {return 0;}
  int _getpid() {return 0;}
  int _fstat() {return 0;}
  int _isatty() {return 0;}
*/


/* void *malloc(size_t n) {return NULL;} */
/* void *calloc(size_t nmemb, size_t size) {return NULL;} */
/* void *realloc(void *ptr, size_t size) {return NULL;} */
/* void free(void *p) {} */
/* int printf(const char *m, ...) {return 0;} */
/* void *memcpy(void *dest, const void *src, size_t n) {return NULL;} */
/* int memcmp(const void *s1, const void *s2, size_t n) {return 0;} */
/* void *memmove(void *dest, const void *src, size_t n) {return NULL;} */
/* void *memset(void *s, int c, size_t n) {return NULL;} */
/* int strcmp(const char *s1, const char* s2) {return 0;} */
/* int strncmp(const char *s1, const char* s2, size_t n) {return 0;} */
/* size_t strlen(const char *s) {return 0;} */
/* char *strcat(char *dest, const char *src) {return NULL;} */
/* char *strchr(const char *dest, int c) {return NULL;} */
/* #include <stdarg.h> */
/* int vprintf(const char *format, va_list ap) {return 0;} */
/* int vsnprintf(char *str,  size_t  size,  const  char  *format, va_list ap) {return 0;} */

/* #undef putchar */
/* int putchar(int c) {return 0;} */
/* int puts(const char *s) {return 0;} */

/* void _start(void) {main(0, NULL);} */
