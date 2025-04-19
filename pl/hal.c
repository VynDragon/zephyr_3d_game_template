#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(PL3D);

extern void *
EXT_calloc(unsigned n, unsigned esz)
{
    return k_calloc(n, esz);
}

extern void
EXT_free(void *p)
{
    k_free(p);
}

extern void
EXT_error(int err_id, char *modname, char *msg)
{
    LOG_ERR("vx error 0x%x in %s: %s\n", err_id, modname, msg);
}
