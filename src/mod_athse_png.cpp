/*
 *
 * An AHTSE module that modifies PNG chunk information
 * Lucian Plesea
 * (C) 2019
 * 
 */

#include <ahtse.h>
#include <receive_context.h>
#include <cctype>

using namespace std;

NS_AHTSE_USE

extern module AP_MODULE_DECLARE_DATA ahtse_png_module;

// Colors, anonymous enum
enum { RED = 0, GREEN, BLUE, ALPHA };

// PNG constants
static const apr_byte_t PNGSIG[8] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
static const apr_byte_t IHDR[4] = { 0x49, 0x48, 0x44, 0x52 };
static const apr_byte_t PLTE[4] = { 0x50, 0x4c, 0x54, 0x45 };
static const apr_byte_t tRNS[4] = { 0x74, 0x52, 0x4e, 0x53 };
static const apr_byte_t IDAT[4] = { 0x49, 0x44, 0x41, 0x54 };
static const apr_byte_t IEND[4] = { 0x49, 0x45, 0x4e, 0x44 };

// Compares two u32, alignemnt and order insensitive
static int is_same_u32(const apr_byte_t *chunk, const apr_byte_t *src) {
    return chunk[0] == src[0] &&
        chunk[1] == src[1] &&
        chunk[2] == src[2] &&
        chunk[3] == src[3];
}

// PNG CRC implementation, slightly modified for C++ from zlib, single table
static apr_uint32_t update_crc32(unsigned char *buf, int len, 
    apr_uint32_t crc = 0xffffffff)
{
    static const apr_uint32_t table[256] = {
        0x00000000UL, 0x77073096UL, 0xee0e612cUL, 0x990951baUL, 0x076dc419UL,
        0x706af48fUL, 0xe963a535UL, 0x9e6495a3UL, 0x0edb8832UL, 0x79dcb8a4UL,
        0xe0d5e91eUL, 0x97d2d988UL, 0x09b64c2bUL, 0x7eb17cbdUL, 0xe7b82d07UL,
        0x90bf1d91UL, 0x1db71064UL, 0x6ab020f2UL, 0xf3b97148UL, 0x84be41deUL,
        0x1adad47dUL, 0x6ddde4ebUL, 0xf4d4b551UL, 0x83d385c7UL, 0x136c9856UL,
        0x646ba8c0UL, 0xfd62f97aUL, 0x8a65c9ecUL, 0x14015c4fUL, 0x63066cd9UL,
        0xfa0f3d63UL, 0x8d080df5UL, 0x3b6e20c8UL, 0x4c69105eUL, 0xd56041e4UL,
        0xa2677172UL, 0x3c03e4d1UL, 0x4b04d447UL, 0xd20d85fdUL, 0xa50ab56bUL,
        0x35b5a8faUL, 0x42b2986cUL, 0xdbbbc9d6UL, 0xacbcf940UL, 0x32d86ce3UL,
        0x45df5c75UL, 0xdcd60dcfUL, 0xabd13d59UL, 0x26d930acUL, 0x51de003aUL,
        0xc8d75180UL, 0xbfd06116UL, 0x21b4f4b5UL, 0x56b3c423UL, 0xcfba9599UL,
        0xb8bda50fUL, 0x2802b89eUL, 0x5f058808UL, 0xc60cd9b2UL, 0xb10be924UL,
        0x2f6f7c87UL, 0x58684c11UL, 0xc1611dabUL, 0xb6662d3dUL, 0x76dc4190UL,
        0x01db7106UL, 0x98d220bcUL, 0xefd5102aUL, 0x71b18589UL, 0x06b6b51fUL,
        0x9fbfe4a5UL, 0xe8b8d433UL, 0x7807c9a2UL, 0x0f00f934UL, 0x9609a88eUL,
        0xe10e9818UL, 0x7f6a0dbbUL, 0x086d3d2dUL, 0x91646c97UL, 0xe6635c01UL,
        0x6b6b51f4UL, 0x1c6c6162UL, 0x856530d8UL, 0xf262004eUL, 0x6c0695edUL,
        0x1b01a57bUL, 0x8208f4c1UL, 0xf50fc457UL, 0x65b0d9c6UL, 0x12b7e950UL,
        0x8bbeb8eaUL, 0xfcb9887cUL, 0x62dd1ddfUL, 0x15da2d49UL, 0x8cd37cf3UL,
        0xfbd44c65UL, 0x4db26158UL, 0x3ab551ceUL, 0xa3bc0074UL, 0xd4bb30e2UL,
        0x4adfa541UL, 0x3dd895d7UL, 0xa4d1c46dUL, 0xd3d6f4fbUL, 0x4369e96aUL,
        0x346ed9fcUL, 0xad678846UL, 0xda60b8d0UL, 0x44042d73UL, 0x33031de5UL,
        0xaa0a4c5fUL, 0xdd0d7cc9UL, 0x5005713cUL, 0x270241aaUL, 0xbe0b1010UL,
        0xc90c2086UL, 0x5768b525UL, 0x206f85b3UL, 0xb966d409UL, 0xce61e49fUL,
        0x5edef90eUL, 0x29d9c998UL, 0xb0d09822UL, 0xc7d7a8b4UL, 0x59b33d17UL,
        0x2eb40d81UL, 0xb7bd5c3bUL, 0xc0ba6cadUL, 0xedb88320UL, 0x9abfb3b6UL,
        0x03b6e20cUL, 0x74b1d29aUL, 0xead54739UL, 0x9dd277afUL, 0x04db2615UL,
        0x73dc1683UL, 0xe3630b12UL, 0x94643b84UL, 0x0d6d6a3eUL, 0x7a6a5aa8UL,
        0xe40ecf0bUL, 0x9309ff9dUL, 0x0a00ae27UL, 0x7d079eb1UL, 0xf00f9344UL,
        0x8708a3d2UL, 0x1e01f268UL, 0x6906c2feUL, 0xf762575dUL, 0x806567cbUL,
        0x196c3671UL, 0x6e6b06e7UL, 0xfed41b76UL, 0x89d32be0UL, 0x10da7a5aUL,
        0x67dd4accUL, 0xf9b9df6fUL, 0x8ebeeff9UL, 0x17b7be43UL, 0x60b08ed5UL,
        0xd6d6a3e8UL, 0xa1d1937eUL, 0x38d8c2c4UL, 0x4fdff252UL, 0xd1bb67f1UL,
        0xa6bc5767UL, 0x3fb506ddUL, 0x48b2364bUL, 0xd80d2bdaUL, 0xaf0a1b4cUL,
        0x36034af6UL, 0x41047a60UL, 0xdf60efc3UL, 0xa867df55UL, 0x316e8eefUL,
        0x4669be79UL, 0xcb61b38cUL, 0xbc66831aUL, 0x256fd2a0UL, 0x5268e236UL,
        0xcc0c7795UL, 0xbb0b4703UL, 0x220216b9UL, 0x5505262fUL, 0xc5ba3bbeUL,
        0xb2bd0b28UL, 0x2bb45a92UL, 0x5cb36a04UL, 0xc2d7ffa7UL, 0xb5d0cf31UL,
        0x2cd99e8bUL, 0x5bdeae1dUL, 0x9b64c2b0UL, 0xec63f226UL, 0x756aa39cUL,
        0x026d930aUL, 0x9c0906a9UL, 0xeb0e363fUL, 0x72076785UL, 0x05005713UL,
        0x95bf4a82UL, 0xe2b87a14UL, 0x7bb12baeUL, 0x0cb61b38UL, 0x92d28e9bUL,
        0xe5d5be0dUL, 0x7cdcefb7UL, 0x0bdbdf21UL, 0x86d3d2d4UL, 0xf1d4e242UL,
        0x68ddb3f8UL, 0x1fda836eUL, 0x81be16cdUL, 0xf6b9265bUL, 0x6fb077e1UL,
        0x18b74777UL, 0x88085ae6UL, 0xff0f6a70UL, 0x66063bcaUL, 0x11010b5cUL,
        0x8f659effUL, 0xf862ae69UL, 0x616bffd3UL, 0x166ccf45UL, 0xa00ae278UL,
        0xd70dd2eeUL, 0x4e048354UL, 0x3903b3c2UL, 0xa7672661UL, 0xd06016f7UL,
        0x4969474dUL, 0x3e6e77dbUL, 0xaed16a4aUL, 0xd9d65adcUL, 0x40df0b66UL,
        0x37d83bf0UL, 0xa9bcae53UL, 0xdebb9ec5UL, 0x47b2cf7fUL, 0x30b5ffe9UL,
        0xbdbdf21cUL, 0xcabac28aUL, 0x53b39330UL, 0x24b4a3a6UL, 0xbad03605UL,
        0xcdd70693UL, 0x54de5729UL, 0x23d967bfUL, 0xb3667a2eUL, 0xc4614ab8UL,
        0x5d681b02UL, 0x2a6f2b94UL, 0xb40bbe37UL, 0xc30c8ea1UL, 0x5a05df1bUL,
        0x2d02ef8dUL
    };

    apr_uint32_t val = crc;
    for (int n = 0; n < len; n++)
        val = table[(val ^ *buf++) & 0xff] ^ (val >> 8);
    return val;
}

// PNG values are always big endian
static void poke_u32be(apr_byte_t *dst, apr_uint32_t val) {
    dst[0] = static_cast<apr_byte_t>((val >> 24) & 0xff);
    dst[1] = static_cast<apr_byte_t>((val >> 16) & 0xff);
    dst[2] = static_cast<apr_byte_t>((val >> 8) & 0xff);
    dst[3] = static_cast<apr_byte_t>(val & 0xff);
}

static apr_uint32_t peek_u32be(const apr_byte_t *src) {
    return
        (static_cast<apr_uint32_t>(src[0]) << 24) +
        (static_cast<apr_uint32_t>(src[1]) << 16) +
        (static_cast<apr_uint32_t>(src[2]) << 8) +
        (static_cast<apr_uint32_t>(src[3]));
}

// The transmitted value is 1's complement
static apr_uint32_t crc32(unsigned char *buf, int len)
{
    return update_crc32(buf, len) ^ 0xffffffff;
}

struct png_conf {
    apr_array_header_t *arr_rxp;
    int indirect;        // Subrequests only
    int only;            // Block non-pngs
    char *source;        // source path
    char *postfix;       // optional postfix
    apr_byte_t *chunk_PLTE;    // the PLTE chunk
    apr_byte_t *chunk_tRNS;    // the tRNS chunk
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

// Parses a byte value, returns the value and advances the *src,
// Sets *src to null on error
static apr_byte_t scan_byte(char **src, int base = 0) {
    char *entry = *src;
    while (isspace(*entry))
        entry++;

    if (isdigit(*entry)) {
        apr_int64_t val = apr_strtoi64(entry, &entry, base);
        if (!errno && val >= 0 && val <= 0xff) {
            *src = entry;
            return static_cast<apr_byte_t>(val);
        }
    }
    // Report an error by setting the src to nullptr
    *src = nullptr;
    return 0;
}

// Entries should be in the order of index values
// Index 0 defaults to 0 0 0 0
static const char *build_palette(apr_array_header_t *entries, apr_byte_t *v, int *len)
{
    int ix = 0; // previous index
    for (int i = 0; i < entries->nelts; i++) {
        char *entry = APR_ARRAY_IDX(entries, i, char *);
        // An entry is: Index Red Green Blue Alpha, white space separated
        int idx = 4 * scan_byte(&entry);
        if (!entry)
            return "Invalid entry format";
        if (idx <= ix && !(ix == 0 && idx == 0))
            return "Entries have to be sorted by index value";
        for (int c = RED; c <= ALPHA; c++) {
            v[idx + c] = scan_byte(&entry);
            if (!entry) {
                if (ALPHA != c)
                    return "Entry parsing error, should be at least 4 space separated byte values";
                v[idx + ALPHA] = 0xff;
            }
        }

        for (int j = ix + 4; j < idx; j += 4) { // Interpolate
            double fraction = static_cast<double>(j - ix) / (idx - ix);
            for (int c = RED; c <= ALPHA; c++)
                v[j + c] = static_cast<apr_byte_t>(0.5 + v[ix + c] 
                    + fraction * (v[idx + c] - v[ix + c]));
        }
        ix = idx;
    }
    *len = ix / 4 + 1;
    return nullptr;
}

// returns the number of entries in the tRNS
static int tRNSlen(const apr_byte_t *arr, int len) {
    while (len && arr[(len - 1) * 4 + ALPHA] == 255)
        len--;
    return len;
}

static const char *configure(cmd_parms *cmd, png_conf *c, const char *fname)
{
    const char *err_message, *line;
    auto kvp = readAHTSEConfig(cmd->temp_pool, fname, &err_message);
    if (!kvp)
        return err_message;
    line = apr_table_get(kvp, "Palette");
    if (line) {
        line = apr_table_getm(cmd->temp_pool, kvp, "Entry");
        if (!line)
            return "Palette requires at least one Entry";
        auto entries = tokenize(cmd->temp_pool, line, ',');
        if (entries->nelts > 256)
            return "Maximum number of entries is 256";
        auto arr = reinterpret_cast<apr_byte_t *>(
            apr_pcalloc(cmd->temp_pool, 256 * 4));
        int len = 0;
        err_message = build_palette(entries, arr, &len);
        if (err_message)
            return err_message;
        // Allocate and convert
        auto chunk_PLTE = reinterpret_cast<apr_byte_t *>(apr_pcalloc(cmd->pool, 3 * len + 12));
        poke_u32be(chunk_PLTE, 3 * len);
        poke_u32be(chunk_PLTE + 4, peek_u32be(PLTE));
        apr_byte_t *p = chunk_PLTE + 8;
        for (int i = 0; i < len * 4; i += 4)
            for (int c = RED; c < ALPHA; c++)
                *p++ = arr[i * 4 + c];
        poke_u32be(chunk_PLTE + 3 * len + 8, crc32(chunk_PLTE + 4, 3 * len + 4));
        c->chunk_PLTE = chunk_PLTE;
        // Same for the tRNS, if needed
        int tlen = tRNSlen(arr, len);
        if (tlen) {
            auto chunk_tRNS = reinterpret_cast<apr_byte_t *>(apr_pcalloc(cmd->pool, tlen + 12));
            poke_u32be(chunk_tRNS, tlen);
            poke_u32be(chunk_tRNS + 4, peek_u32be(tRNS));
            apr_byte_t *p = chunk_tRNS + 8;
            for (int i = 0; i < tlen; i++)
                *p++ = arr[i * 4 + ALPHA];
            poke_u32be(chunk_PLTE + 3 * len + 8, crc32(chunk_PLTE + 4, len + 4));
            c->chunk_tRNS = chunk_tRNS;
        }
    }
    return nullptr;
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

static void register_hooks(apr_pool_t *p) {
    ap_hook_handler(handler, NULL, NULL, APR_HOOK_LAST);
}

static const command_rec cmds[] = {
    AP_INIT_TAKE1(
        "AHTSE_PNG_RegExp",
        (cmd_func) set_regexp,
        0, // self pass arg, added to the config address
        ACCESS_CONF,
        "The request pattern the URI has to match"
    )

    ,AP_INIT_TAKE12(
        "AHTSE_PNG_Source",
        (cmd_func) set_source<png_conf>,
        0,
        ACCESS_CONF,
        "Required, internal path for the source. Optional postfix is space separated"
    )

    ,AP_INIT_FLAG(
        "AHTSE_PNG_Indirect",
        (cmd_func) ap_set_flag_slot,
        (void *)APR_OFFSETOF(png_conf, indirect),
        ACCESS_CONF, // availability
        "If set, module only activates on subrequests"
    )

    ,AP_INIT_TAKE1(
        "AHTSE_PNG_ConfigurationFile",
        (cmd_func) configure,
        0,
        ACCESS_CONF,
        "File holding the AHTSE_PNG configuration"
    )

    ,AP_INIT_FLAG(
        "AHTSE_PNG_Only",
        (cmd_func) ap_set_flag_slot,
        (void *)APR_OFFSETOF(png_conf, only),
        ACCESS_CONF, // availability
        "If set, non-png files are blocked and reported as warnings."
        " Default is off, allowing them to be sent"
    )

    ,{NULL}
};

module AP_MODULE_DECLARE_DATA ahtse_png_module = {
    STANDARD20_MODULE_STUFF,
    create_dir_config,
    0,  // merge_dir_config
    0,  // create_server_config
    0,  // merge_server_config
    cmds,
    register_hooks
};
