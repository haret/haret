#include <windows.h> // TlsSetValue

#include "output.h" // Output

#include "exceptions.h"

static DWORD handler_tls;

void start_ehandling(struct eh_data *d)
{
    d->old_handler = (struct eh_data*)TlsGetValue(handler_tls);
    TlsSetValue(handler_tls, d);
}

void end_ehandling(struct eh_data *d)
{
    TlsSetValue(handler_tls, d->old_handler);
}

void init_thread_ehandling(void)
{
    TlsSetValue(handler_tls, NULL);
}

void init_ehandling(void)
{
    handler_tls = TlsAlloc();
}

extern "C" EXCEPTION_DISPOSITION
eh_handler(struct _EXCEPTION_RECORD *ExceptionRecord,
           void *EstablisherFrame,
           struct _CONTEXT *ContextRecord,
           struct _DISPATCHER_CONTEXT *DispatcherContext)
{
    struct eh_data *d = (struct eh_data*)TlsGetValue(handler_tls);
    if (! d) {
        Output(C_ERROR "Terminating haret due to unhandled exception"
               " (pc=%08lx)", ContextRecord->Pc);
        exit(1);
    }
    end_ehandling(d);

    ContextRecord->Pc = (ulong)longjmp;
    ContextRecord->R0 = (ulong)d->env;
    ContextRecord->R1 = 1;
    return ExceptionContinueExecution;
}

__asm__(
    // Data to be placed at start of .text section
    "\t.section .init\n"
    "\t.word eh_handler\n"
    "\t.word 0\n"
    "start_eh_text:\n"

    // Data for exception handler
    "\t.section .pdata\n"
    "\t.word start_eh_text\n"
    "\t.word 0xc0000002 | (0xFFFFF << 8)\n" // max 22 bits for number of instructions

    "\t.text\n"
    );
