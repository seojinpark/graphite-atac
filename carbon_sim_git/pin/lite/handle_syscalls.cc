#include <syscall.h>
using namespace std;

#include "lite/handle_syscalls.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "syscall_model.h"
#include "log.h"

namespace lite
{

void handleFutexSyscall(CONTEXT* ctx)
{
   ADDRINT syscall_number = PIN_GetContextReg (ctx, REG_GAX);
   if (syscall_number != SYS_futex)
      return;

   SyscallMdl::syscall_args_t args;

#ifdef TARGET_IA32
   args.arg0 = PIN_GetContextReg (ctx, REG_GBX);
   args.arg1 = PIN_GetContextReg (ctx, REG_GCX);
   args.arg2 = PIN_GetContextReg (ctx, REG_GDX);
   args.arg3 = PIN_GetContextReg (ctx, REG_GSI);
   args.arg4 = PIN_GetContextReg (ctx, REG_GDI);
   args.arg5 = PIN_GetContextReg (ctx, REG_GBP);
#endif

#ifdef TARGET_X86_64
   // FIXME: The LEVEL_BASE:: ugliness is required by the fact that REG_R8 etc 
   // are also defined in /usr/include/sys/ucontext.h
   args.arg0 = PIN_GetContextReg (ctx, LEVEL_BASE::REG_GDI);
   args.arg1 = PIN_GetContextReg (ctx, LEVEL_BASE::REG_GSI);
   args.arg2 = PIN_GetContextReg (ctx, LEVEL_BASE::REG_GDX);
   args.arg3 = PIN_GetContextReg (ctx, LEVEL_BASE::REG_R10); 
   args.arg4 = PIN_GetContextReg (ctx, LEVEL_BASE::REG_R8);
   args.arg5 = PIN_GetContextReg (ctx, LEVEL_BASE::REG_R9);
#endif

   Core* core = Sim()->getTileManager()->getCurrentCore();
   
   LOG_ASSERT_ERROR(core != NULL, "Core(NULL)");

   LOG_PRINT("Enter Syscall (202)");
   
   core->getSyscallMdl()->runEnter(syscall_number, args);
}

void syscallEnterRunModel(THREADID threadIndex, CONTEXT* ctx, SYSCALL_STANDARD syscall_standard, void* v)
{
   Core* core = Sim()->getTileManager()->getCurrentCore();
   LOG_ASSERT_ERROR(core, "Core(NULL)");

   IntPtr syscall_number = PIN_GetSyscallNumber(ctx, syscall_standard);
   
   if (syscall_number != SYS_futex)
      LOG_PRINT("Enter Syscall(%i)", (int) syscall_number);

   // Save the syscall number
   core->getSyscallMdl()->saveSyscallNumber(syscall_number);
   if (syscall_number == SYS_futex)
   {
      PIN_SetSyscallNumber(ctx, syscall_standard, SYS_getpid);
   }
}

void syscallExitRunModel(THREADID threadIndex, CONTEXT* ctx, SYSCALL_STANDARD syscall_standard, void* v)
{
   Core* core = Sim()->getTileManager()->getCurrentCore();
   LOG_ASSERT_ERROR(core, "Core(NULL)");

   IntPtr syscall_number = core->getSyscallMdl()->retrieveSyscallNumber();
   IntPtr syscall_return = PIN_GetSyscallReturn(ctx, syscall_standard);

   if (syscall_number == SYS_futex)
   {
      IntPtr old_return_val = syscall_return;
      syscall_return = core->getSyscallMdl()->runExit(old_return_val);
      PIN_SetContextReg(ctx, REG_GAX, syscall_return);

   }

   LOG_PRINT("Exit Syscall(%i) - Return Value(%#llx)", (int) syscall_number, (long long unsigned int) syscall_return);
}

}
