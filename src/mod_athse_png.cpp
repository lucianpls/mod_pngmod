/*
 *
 * An AHTSE module that modifies PNG chunk information
 * Lucian Plesea
 * (C) 2019
 * 
 */

#include <ahtse.h>
#include <receive_context.h>

using namespace std;
NS_AHTSE_USE

#define CMD_FUNC (cmd_func)
struct png_conf {
    apr_array_header_t *arr_rxp;
};

static void *create_dir_config(apr_pool_t *p, char *dummy) {
    png_conf *c = reinterpret_cast<png_conf *>(
        apr_pcalloc(p, sizeof(png_conf)));
    return c;
}

static const command_rec cmds[] = {
    {NULL}
};

static int handler(request_rec *r) {
    return DECLINED;
}

static void register_hooks(apr_pool_t *p) {
    ap_hook_handler(handler, NULL, NULL, APR_HOOK_LAST);
}

module AP_MODULE_DECLARE_DATA ahtse_png_module = {
    STANDARD20_MODULE_STUFF,
    create_dir_config,
    0,  // merge_dir_config
    0,  // create_server_config
    0,  // merge_server_config
    cmds,
    register_hooks
};
