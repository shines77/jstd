
#include "common.h"

static void jstd_init(void) {}
static void jstd_exit(void) {}

#if defined(SMP) && defined(USE_TLS)
static void blas_thread_memory_cleanup(void) {}
#endif

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            jstd_init();
            break;

        case DLL_PROCESS_DETACH:
            // If the process is about to exit, don't bother releasing any resources
            // The kernel is much better at bulk releasing then.
            if (!reserved) {
                jstd_exit();
            }
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
#if defined(SMP) && defined(USE_TLS)
            jstd_thread_memory_cleanup();
#endif
            break;
    }

    return TRUE;
}
