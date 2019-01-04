#include "autocomplete.h"
#include "ff.h"
#include "ffdir.h"
#include <string.h>
#include <stdio.h>
#include "xprintf.h"

static
void printlist(clist_t *cl){
  int i;
  if(cl->length){
    for(i=0;i<cl->length;i++){
      xprintf("%-13s",cl->items[i].name);
    }
  }else{
    i=1;
    xprintf("%-13s","no hit");
  }
  for(;i<_MAX_COMPLETE_LENGTH;i++){
    xprintf("%13s","");
  }
}

static
void csearch(char *name,ctype_t *type,clist_t *cl){
  FRESULT res;
  DIR dr;
  FILINFO fno;
  int i=0,j;
  char tmp[16];
  char *filepart;
  char path[_MAX_PATH_LENGTH];

  strcpy(path,name);
  filepart = path;
  while(*filepart)filepart++;
  while(path!=filepart&&*filepart!='/')filepart--;
  if(path!=filepart){
    *filepart='\0';
    filepart++;
    /* xprintf("name(%s)  path:(%s)  pattern:(%s)\n\n",name,path,filepart); */
    strcpy(tmp,filepart);
    strcat(tmp,"*");
    /* xprintf("\nsearch for [%s]\n",name); */
    res = f_findfirst(&dr,&fno,path,tmp);
  }else{//no dir
    /* xprintf("name(%s)  path:.  pattern:(%s)\n\n",name,filepart); */
    strcpy(tmp,filepart);
    strcat(tmp,"*");
    /* xprintf("\nsearch for [%s]\n",name); */
    res = f_findfirst(&dr,&fno,"",tmp);
  }

  
  while(res == FR_OK&&fno.fname[0]&&i<_MAX_COMPLETE_LENGTH){
    cl->items[i].attr = fno.fattrib;
    strcpy(cl->items[i].name,fno.fname);
    if(fno.fattrib&AM_DIR){
      if(cl->items[i].name[0] == '.'){
	i--;//ignore
      }else{
	strcat(cl->items[i].name,"/");
      }
    }else if(type->attr == AM_DIR){
      i--;//ignore
    }
    i++;
    res = f_findnext(&dr, &fno);
  }
  cl->length = i;
  if(i==0){
    printlist(cl);
  }else if(i==1){
    if(path!=filepart){
      strcat(path,"/");
      strcat(path,cl->items[0].name);
      strcpy(name,path);
    }else{
      strcpy(name,cl->items[0].name);
    }
  }else{
    strcpy(cl->matchpart,cl->items[0].name);
    for(i=1;i<cl->length;i++){
      for(j=0;cl->matchpart[j]==cl->items[i].name[j];j++);
      cl->matchpart[j] = 0;
    }
    /* xprintf("*match:(%s)\n",cl->matchpart); */
    if(path!=filepart){
      strcat(path,"/");
      strcat(path,cl->matchpart);
      strcpy(name,path);
    }else{
      strcpy(name,cl->matchpart);
    }    
    printlist(cl);
  }
}

/*name length must be good enough! :) */
int inputfilepath(char *name, ctype_t *type,const char *msg){
  clist_t lst;

  char m;
  char *sb = name;
  int eofilename = 0;
  while(!eofilename){
    xprintf("\033[%d;%dH%s %s\033[0K",_N_LINE+1+3,1,msg,name);
    m = getchar();
    switch(m){
    case '\n':
      eofilename = 1;
      break;
    case '\t':
      xprintf("\033[%d;1H",1);
      csearch(name,type,&lst);
      sb = name;
      while(*sb)sb++;
      break;
    case '\x7F':
      if(sb != name){
	sb--;
	*sb = 0;
      }
      break;
    default:
      *sb++ = m;
      *sb = 0;
    }
  }
  return 0;
}
