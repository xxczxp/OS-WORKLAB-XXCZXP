#include "co.h"
#include <stdlib.h>
#include  <setjmp.h>
#include <stdint.h>

#define STACK_SIZE 16000

enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
  char *name;
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  enum co_status status;  // 协程的状态
  struct co *    waiter;  // 是否有其他协程在等待当前协程
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t        stack[STACK_SIZE]; // 协程的堆栈

  //all thread are stored by a two way linklist
  struct co * left;
  struct co * right;
};


struct co * currentCo;

struct co * creatMainCo(void){
  struct co * mainCo = malloc(sizeof(struct co));
  mainCo->name="main";
  mainCo->status=CO_RUNNING;
  mainCo->waiter=NULL;
  mainCo->left=mainCo;
  mainCo->right=mainCo;
  return mainCo;

}

void insertCo(struct co * linkList,struct co * task){
  struct co * next=linkList->right;

  task->left=linkList;
  task->right=next;
  linkList->right=task;
  next->left=task;
}

void deleteCo(struct co * e){
  struct co * left=e->left;
  struct co * right=e->right;
  left->right=right;
  right->left=left;

  free((void*)e);
}


static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
      : : "b"((uintptr_t)sp), "d"(entry), "a"(arg) : "memory"
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg) : "memory"
#endif
  );
}

//this function shouldn't be called
void runningEndCo(void){
  currentCo->status=CO_DEAD;
  co_yield();
}

void excuteCo(struct co * co){
#if __x86_64__
  void* s_top=&(co->stack[(STACK_SIZE/8-1)*8]);
  *((void * *)s_top)=&runningEndCo;
  stack_switch_call(s_top,co->func,co->arg);
#else //TODO for 32 machine 
    
#endif
  }

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  static int isFirst=1;
  if(isFirst==0){
    currentCo=creatMainCo();
    isFirst=0;
  }

  struct co * newCo = malloc(sizeof(struct co));
  newCo->name=name;
  newCo->status=CO_NEW;
  newCo->waiter=NULL;
  newCo->func=func;
  newCo->arg=arg;
  insertCo(currentCo,newCo);
  
  return newCo;

    
}

void co_wait(struct co *co) {
  
}

void co_yield() {
  if(!setjmp(currentCo->context)){
    struct co * p=currentCo;
    while ((p=p->right)!=currentCo)
    {
      if(p->status==CO_RUNNING){

      }
    }
    
  }
}
