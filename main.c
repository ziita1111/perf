#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

const int event_num = 7;

struct Event{
  char name[30];
  unsigned long event_select;
  unsigned long umask;
  int enable;
};

struct Event events[7] = {
{"UnHalted Core Cycles",0x3c,0,0}, 
{"Instruction Retired",0xc0,0,0},
{"Unhalted Reference Cycles",0x3c,0x1,0},
{"LLC Reference",0x2e,0x4f,0},
{"LLC Misses",0x2e,0x41,0},
{"Branch Instruction Retired",0xc4,0,0},
{"Branch Misses Retired",0xc5,0,0},
};

void cpuid(int *eax, int *ebx, int *ecx, int *edx){
  __asm __volatile(
    "cpuid"
    : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
    : "0"(*eax), "2"(*ecx)
    : "memory"
  );
}

void init_events(){
  unsigned int eax,ebx,ecx,edx;
  eax = 0xa;
  cpuid(&eax,&ebx,&ecx,&edx);
  for(int i = 0; i < event_num; i++){
    if((ebx>>i)%2==0){
      events[i].enable=1;
    }
  }  
}

void set_msr(int idx){
  unsigned long flag = 0;
  unsigned long base = 0;
  unsigned long none = 0;
  flag |= events[idx].event_select | events[idx].umask<<8;
  flag |= 1<<16; // USR flag
  flag |= 1<<17; // OS flag
  flag |= 1<<20; // INT flag
  flag |= 1<<22; // EN flag
  base = 0x186; // IA32_PERFEVTSELx MSR

  int fd;
  char* msr_file_name = "/dev/cpu/0/msr";
  fd = open(msr_file_name, O_WRONLY);
  if(fd == -1){
    perror("open");
    exit(1);
  }
  if(pwrite(fd, &flag, sizeof flag, base)!=sizeof flag){
      exit(1);
  }
  close(fd);
}

unsigned long read_msr(int idx){
  unsigned int base_rdpmc = 0xc1; // IA32_PMCx MSRs
  unsigned long data;
  int fd;
  char* msr_file_name = "/dev/cpu/0/msr";
  fd = open(msr_file_name, O_RDONLY);
  if(fd == -1){
    perror("open");
    exit(1);
  }
  if(pread(fd, &data, sizeof data, base_rdpmc) != sizeof data){
    exit(1);
  }
  close(fd);
  return data;
}

void main(int argc, char** argv){
  init_events();
  for(int i = 0; i < event_num; i++){
    if(!events[i].enable){
      printf("%s: unsupported\n", events[i].name);
      continue;
    }
    set_msr(i);
    unsigned long pre = read_msr(i);
    sleep(2);
    unsigned long post = read_msr(i);
    printf("%s: %ld\n", events[i].name, post-pre);
  }
}
