#include  "system.h"

#define ESCAPE_KEY 0x1b
#define BACKSP_KEY 0x08
#define TAB_KEY    0x09
#define RETURN_KEY 0x0D
#define DELETE_KEY 0x7F
#define BELL       0x07

static int32_t cli_handler(void *arg);
static void print_prompt();
bool util_cli_freertos_init(char *(*prompt_func)() , int (*run_command)(char*) );
extern void util_cli_freertos_callback(char c);
extern bool g_cli_typing;
