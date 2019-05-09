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

extern module AP_MODULE_DECLARE_DATA ahtse_png_module;

struct png_conf {
    apr_array_header_t *arr_rxp;
    int indirect;  // Subrequests only
    int only;      // Block non-pngs
};

static void *create_dir_config(apr_pool_t *p, char *dummy) {
    png_conf *c = reinterpret_cast<png_conf *>(
        apr_pcalloc(p, sizeof(png_conf)));
    return c;
}

static const char *set_regexp(cmd_parms *cmd, png_conf *c, const char *pattern)
{
    return add_regexp_to_array(cmd->pool, &c->arr_rxp, pattern);
}

static int handler(request_rec *r) {
    if (r->method_number != M_GET)
        return DECLINED;

    png_conf *cfg = get_conf<png_conf>(r, &ahtse_png_module);
    if ((cfg->indirect && !r->main)
        || !requestMatches(r, cfg->arr_rxp))
        return DECLINED;

    // Our request
    sz tile;
    if (APR_SUCCESS != getMLRC(r, tile))
        return HTTP_BAD_REQUEST;

    return DECLINED;
}

static const command_rec cmds[] = {
    AP_INIT_TAKE1(
        "AHTSE_PNG_RegExp",
        CMD_FUNC set_regexp,
        0, // self pass arg
        ACCESS_CONF,
        "The request pattern the URI has to match"
    )
    ,AP_INIT_FLAG(
        "AHTSE_PNG_Indirect",
        CMD_FUNC ap_set_flag_slot,
        (void *)APR_OFFSETOF(png_conf, indirect),
        ACCESS_CONF, // availability
        "If set, module only activates on subrequests"
    )
    ,AP_INIT_FLAG(
        "AHTSE_PNG_Only",
        CMD_FUNC ap_set_flag_slot,
        (void *)APR_OFFSETOF(png_conf, only),
        ACCESS_CONF, // availability
        "If set, non-png files are blocked and reported as warnings."
        " Default is off, allowing them to be sent"
    )
    ,{NULL}
};

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
