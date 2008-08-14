/**
	\file kernel/ptrace.h
	\brief Bibliotecas para la syscall Ptrace
	\todo hacer un ptrace.h y ponerlos ahi e incluir ese en este archivo igual que esta con signal.h
	\todo completar documentacion
*/
#ifndef __PTRACE_H
#define   __PTRACE_H


#define PTRACE_TRACEME             0
#define PTRACE_PEEKTEXT            1
#define PTRACE_PEEKDATA            2
#define PTRACE_PEEKUSR             3
#define PTRACE_POKETEXT            4
#define PTRACE_POKEDATA            5
#define PTRACE_POKEUSR             6
#define PTRACE_CONT                7
#define PTRACE_KILL                8
#define PTRACE_SINGLESTEP          9
#define PTRACE_ATTACH             10
#define PTRACE_DETACH             11
#define PTRACE_GETREGS            12
#define PTRACE_SETREGS            13
#define PTRACE_GETFPREGS          14
#define PTRACE_SETFPREGS          15
#define PTRACE_GETFPXREGS         18
#define PTRACE_SETFPXREGS         19
#define PTRACE_OLDSETOPTIONS      21
#define PTRACE_SYSCALL            24
#define PTRACE_GET_THREAD_AREA    25
#define PTRACE_SET_THREAD_AREA    26
#define PTRACE_SYSEMU             31
#define PTRACE_SYSEMU_SINGLESTEP  32 


#define EBX 4
#define ECX 8
#define EDX 12
#define ESI 16
#define EDI 20
#define EBP 24
#define EAX 28
#define DS 32
#define ES 34
#define ORIG_EAX 36
#define EIP 40
#define CS  44
#define EFL 46
#define UESP 50
//#define SS   56
//#define FRAME_SIZE 17
#define DATA 56
#define ADDR 60


struct stuParametros{
int pid,ebx,ecx,edx,esi,edi,ebp,eax,xds,xes,eip,xcs,eflags,esp;
int data;
void * addr;
};

struct pt_regs {
 long ebx;
 long ecx;
 long edx;
 long esi;
 long edi;
 long ebp;
 long eax;
 int  xds;
 int  xes;
 long orig_eax;
 long eip;
 int  xcs;
 long eflags;
 long esp;
 int  xss;
};

#endif // __PTRACE_H
