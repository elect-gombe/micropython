/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include "py/runtime.h"

#include "extmod/vfs_fat.h"
#include <string.h>
#include "NVMem.h"

#define _FLASH_PAGE_SIZE 4096

#define _BUFFER_SIZE_KiB 80

const uint8_t flash_buffer[_BUFFER_SIZE_KiB*1024] __attribute__((aligned(_FLASH_PAGE_SIZE)))={
#include "makefat/fat.csv"
};

#define SECTOR_SIZE 512

#define FLASH_PART1_START_BLOCK (0x100)
#define FLASH_PART1_NUM_BLOCKS (_BUFFER_SIZE_KiB*2)
#define FLASH_ROW_SIZE 512
#define printf(...) mp_printf(&mp_plat_print,__VA_ARGS__);wait60thsec(60);

static void build_partition(uint8_t *buf, int boot, int type, uint32_t start_block, uint32_t num_blocks) {
  buf[0] = boot;

  if (num_blocks == 0) {
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
  } else {
    buf[1] = 0xff;
    buf[2] = 0xff;
    buf[3] = 0xff;
  }

  buf[4] = type;

  if (num_blocks == 0) {
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 0;
  } else {
    buf[5] = 0xff;
    buf[6] = 0xff;
    buf[7] = 0xff;
  }

  buf[8] = start_block;
  buf[9] = start_block >> 8;
  buf[10] = start_block >> 16;
  buf[11] = start_block >> 24;

  buf[12] = num_blocks;
  buf[13] = num_blocks >> 8;
  buf[14] = num_blocks >> 16;
  buf[15] = num_blocks >> 24;
}

static
int sector_read(uint8_t *buffer,const unsigned int page,const unsigned int number_of_sector){
  if(page == 0){
    memset(buffer,0,446);
    build_partition(buffer + 446, 0, 0x01 /* FAT12 */, FLASH_PART1_START_BLOCK, FLASH_PART1_NUM_BLOCKS);
    build_partition(buffer + 462, 0, 0, 0, 0);/* empty */
    build_partition(buffer + 478, 0, 0, 0, 0);
    build_partition(buffer + 494, 0, 0, 0, 0);
    buffer[510] = 0x55;
    buffer[511] = 0xaa;
    return 0;
  } else if (page-FLASH_PART1_START_BLOCK < _BUFFER_SIZE_KiB*2){
    memcpy(buffer,flash_buffer+(page-FLASH_PART1_START_BLOCK)*SECTOR_SIZE,number_of_sector*SECTOR_SIZE);
    return 0;
  } else {
    return 1;
  }
}

#if 1 //
// check if itself is blank,  erase accordingly, then write.

static
int blankcheck(const void *buff,int len){
  int i;
  for(i=0;i<len&&((uint32_t*)buff)[i]==0xFFFFFFFF;i++);

  return i!=len;//0..pass other..erase required
}

int writerow(const void *buff,const void*p){
  uint32_t u;

  u=(uint32_t)p;
  if(u >= (uint32_t)flash_buffer&&
     u < (uint32_t)flash_buffer + _BUFFER_SIZE_KiB*1024&&
     (u&(512-1))==0){
    int ret = NVMemWriteRow(p,buff);
    if(ret){
      //printf("write err\n (%p)\n",p);
    }
    return ret;
  }else{
    //printf("write err\n wrong addr(%p)",p);
    //printf("(%p~%p)\n",flash_buffer,flash_buffer+_BUFFER_SIZE_KiB*1024);
    return 1;
  }
}

static
int write1page(const void *buff,const void *flashaddr){
  int ret;
  const void *b = buff;
  do{
    ret = writerow(b,flashaddr);
    b += 512,flashaddr += 512;
  }while(ret==0&&b!=buff+_FLASH_PAGE_SIZE);
  return ret;
}

static
int erasepage(const void *p){
  uint32_t u;
  u=(uint32_t)p;
  if(u >= (uint32_t)flash_buffer&&
     u < (uint32_t)flash_buffer + _BUFFER_SIZE_KiB*1024&&
     (u&(_FLASH_PAGE_SIZE-1))==0){
    int r=NVMemErasePage(p);
    if(r){
      //printf("erase err(%d)",r);
      return 1;
    }
  }else{
    //printf("erase err\n wrong addr(%p)",p);
    //printf("(%p~%p)\n",flash_buffer,flash_buffer+_BUFFER_SIZE_KiB*1024);
    return 1;
  }
  return 0;
}

int sector_write(const uint8_t *buff,const unsigned int page,int n){
  const void *flash_addr;
  const void *flash_page;
  int iscontinue = 1;
  n*=512;
  flash_addr = (page-FLASH_PART1_START_BLOCK) * SECTOR_SIZE + flash_buffer;
  flash_page = ((uint32_t)flash_addr)&(~(_FLASH_PAGE_SIZE-1));// align to flash page size
  if (page >= FLASH_PART1_START_BLOCK&&page-FLASH_PART1_START_BLOCK < _BUFFER_SIZE_KiB*2);
  else return 1;
  while(iscontinue){
    int pp = (uint32_t)flash_addr-(uint32_t)flash_page;// offset within this page.
    int nc = _FLASH_PAGE_SIZE - pp;
    if(nc >= n){//limit 1 page write at the same time.
      nc = n;
      iscontinue = 0;
    }
    //printf("n:%05x,pp:%05x,nc:%05x\n",n,pp,nc);
    //printf("buf:%p,page:%p,adr%p",buff,flash_page,flash_addr);
    if(blankcheck(flash_addr,nc)){
      //printf("erase required\n");
      //let it erase
      //bkup, be sure that enough stack space is available(at least flash page + alpha)
      uint32_t b[_FLASH_PAGE_SIZE/4];
      memcpy(b,flash_page,_FLASH_PAGE_SIZE);
      memcpy(((uint8_t*)b)+pp,buff,nc);
      int ret = erasepage(flash_page);
      if(ret){
	return ret;
      }
      ret=write1page(b,flash_page);
      if(ret!=0){
        //printf("write err\n");
	return ret;
      }
      flash_addr+=nc;
      buff+=nc;
      n-=nc;
    }else{
      //No erase needed.
      do{
	int ret = writerow(buff,flash_addr);
	if(ret!=0){
	  //printf("write err\n");
	  return ret;
	}
	flash_addr+=512;
	buff+=512;
	n-=512;
	nc-=SECTOR_SIZE;
      }while(nc);
    }
    flash_page += _FLASH_PAGE_SIZE;
  }
  //printf("exit success\n");
  return 0;
}
//MachiKaniaで使われているps2keyboard.cをできれば公開してもらえますか。それとももしかしてhttp://www.ze.em-net.ne.jp/~kenken/ps2kb/index.htmlの中身と同じですか？　micropythonのCtrl-Cの中断の実装中で、割り込みハンドラ内少し書き換えたいです。
/* static */
/* int sector_write(const unsigned int page,unsigned int number_of_sector,const uint8_t *buffer){ */
/*   int ret; */
/*   uint8_t flash_temp_buffer[1024] __attribute__((aligned(4))); */

/*   if(page == 0||number_of_sector == 0){ */
/*     return 0; */
/*   } else if (page-FLASH_PART1_START_BLOCK < _BUFFER_SIZE_KiB*2){ */
/*     const void* flash_addr; */
/*     if((unsigned int)flash_addr & SECTOR_SIZE){//sector alignment required! */
/*       memcpy(flash_temp_buffer,flash_addr-SECTOR_SIZE,SECTOR_SIZE); */
/*       memcpy(flash_temp_buffer+SECTOR_SIZE,buffer,SECTOR_SIZE); */
/*       ret = NVMemErasePage(flash_addr-SECTOR_SIZE); */
/*       if(ret)return ret; */
/*       ret = write1page(flash_temp_buffer,flash_addr-SECTOR_SIZE); */
/*       if(ret)return ret; */
/*       if(number_of_sector!=1)sector_write(page+1, number_of_sector-1, buffer+SECTOR_SIZE); */
/*     }else if(number_of_sector == 1){//last write */
/*       memcpy(flash_temp_buffer,buffer,SECTOR_SIZE); */
/*       memcpy(flash_temp_buffer+SECTOR_SIZE,flash_addr+SECTOR_SIZE,SECTOR_SIZE); */
/*       ret = NVMemErasePage(flash_addr); */
/*       if(ret)return ret; */
/*       ret = write1page(flash_temp_buffer,flash_addr); */
/*       if(ret)return ret; */
/*     }else{//2sectors aligned */
/*       memcpy(flash_temp_buffer,buffer,SECTOR_SIZE*2); */
/*       ret = NVMemErasePage(flash_addr); */
/*       if(ret)return ret; */
/*       ret = write1page(flash_temp_buffer,flash_addr); */
/*       if(ret)return ret; */
/*       if(number_of_sector!=1)sector_write(page+2, number_of_sector-2, buffer+SECTOR_SIZE*2); */
/*     } */
/*   } */
/*   return 0; */
/* } */
#endif

/* DRESULT disk_ioctl_flash ( */
/* 	uint8_t ctrl,		/\* Control code *\/ */
/* 	void    *buff		/\* Buffer to send/receive control data *\/ */
/* ) */
/* { */
/*     if(ctrl == GET_SECTOR_COUNT){ */
/*         *(DWORD*)buff = _BUFFER_SIZE_KiB*2; */
/*     }else if(ctrl == GET_SECTOR_SIZE){ */
/*         *(DWORD*)buff = 512; */
/*     }else if(ctrl == GET_BLOCK_SIZE){ */
/*         *(DWORD*)buff = 2; */
/*     } */
/*     return 0; */
/* } */

/* DSTATUS disk_status_flash(void){ */
/*     return 0; */
/* } */

/* DRESULT disk_read_flash ( BYTE* buff, DWORD sector, UINT count){ */
/*     sector_read(sector,count,buff); */
/*     return 0; */
/* } */

/* DRESULT disk_write_flash (const BYTE* buff, DWORD sector, UINT count){ */
/*     sector_write(sector,count,buff); */
/*     return 0; */
/* } */

/* DSTATUS disk_initialize_flash (void){ */
/*     return 0; */
/* } */

/******************************************************************************/
// MicroPython bindings
//
// Expose the flash as an object with the block protocol.

// there is a singleton Flash object
extern const mp_obj_type_t pyb_flash_type;
STATIC const mp_obj_base_t pyb_flash_obj = {&pyb_flash_type};

STATIC mp_obj_t pyb_flash_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // return singleton object
    return MP_OBJ_FROM_PTR(&pyb_flash_obj);
}

STATIC mp_obj_t pyb_flash_readblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);
    mp_uint_t ret = sector_read(bufinfo.buf,mp_obj_get_int(block_num), bufinfo.len / 512);
    return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_flash_readblocks_obj, pyb_flash_readblocks);

STATIC mp_obj_t pyb_flash_writeblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);
    mp_uint_t ret = 0;//storage_write_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / FLASH_BLOCK_SIZE); todo
    return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_flash_writeblocks_obj, pyb_flash_writeblocks);

static
uint32_t storage_get_block_size(void) {
  return 512;
}

static
uint32_t storage_get_block_count(void) {
  return FLASH_PART1_START_BLOCK + FLASH_PART1_NUM_BLOCKS;
}


STATIC mp_obj_t pyb_flash_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    switch (cmd) {
        case BP_IOCTL_INIT: return MP_OBJ_NEW_SMALL_INT(0);
        case BP_IOCTL_DEINIT: return MP_OBJ_NEW_SMALL_INT(0); // TODO properly
        case BP_IOCTL_SYNC: return MP_OBJ_NEW_SMALL_INT(0);
        case BP_IOCTL_SEC_COUNT: return MP_OBJ_NEW_SMALL_INT(storage_get_block_count());
        case BP_IOCTL_SEC_SIZE: return MP_OBJ_NEW_SMALL_INT(storage_get_block_size());
        default: return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_flash_ioctl_obj, pyb_flash_ioctl);

STATIC const mp_rom_map_elem_t pyb_flash_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_readblocks), MP_ROM_PTR(&pyb_flash_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&pyb_flash_writeblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&pyb_flash_ioctl_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_flash_locals_dict, pyb_flash_locals_dict_table);

const mp_obj_type_t pyb_flash_type = {
    { &mp_type_type },
    .name = MP_QSTR_Flash,
    .make_new = pyb_flash_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_flash_locals_dict,
};

void pyb_flash_init_vfs(fs_user_mount_t *vfs) {
    vfs->base.type = &mp_fat_vfs_type;
    vfs->flags |= FSUSER_NATIVE | FSUSER_HAVE_IOCTL;
    vfs->fatfs.drv = vfs;
    vfs->fatfs.part = 1; // flash filesystem lives on first partition
    vfs->readblocks[0] = MP_OBJ_FROM_PTR(&pyb_flash_readblocks_obj);
    vfs->readblocks[1] = MP_OBJ_FROM_PTR(&pyb_flash_obj);
    vfs->readblocks[2] = MP_OBJ_FROM_PTR(sector_read); // native version
    vfs->writeblocks[0] = MP_OBJ_FROM_PTR(&pyb_flash_writeblocks_obj);
    vfs->writeblocks[1] = MP_OBJ_FROM_PTR(&pyb_flash_obj);
    vfs->writeblocks[2] = MP_OBJ_FROM_PTR(sector_write); // native version
    vfs->u.ioctl[0] = MP_OBJ_FROM_PTR(&pyb_flash_ioctl_obj);
    vfs->u.ioctl[1] = MP_OBJ_FROM_PTR(&pyb_flash_obj);
}
