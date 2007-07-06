#include <setjmp.h> // jmp_buf

struct eh_data {
    jmp_buf env;
    struct eh_data *old_handler;
};

#define TRY_EXCEPTION_HANDLER                   \
    struct eh_data __ehd;                       \
    int __ret = setjmp(__ehd.env);              \
    if (!__ret) {                               \
        start_ehandling(&__ehd);

#define CATCH_EXCEPTION_HANDLER                 \
        end_ehandling(&__ehd);                  \
    } else

void start_ehandling(struct eh_data *d);
void end_ehandling(struct eh_data *d);
void init_thread_ehandling(void);
void init_ehandling(void);
