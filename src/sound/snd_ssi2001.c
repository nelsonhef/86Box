#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#define HAVE_STDARG_H

#include <86box/86box.h>
#include <86box/device.h>
#include <86box/gameport.h>
#include <86box/io.h>
#include <86box/snd_resid.h>
#include <86box/sound.h>
#include <86box/plat_unused.h>

typedef struct ssi2001_t {
    void   *psid;
    int16_t buffer[SOUNDBUFLEN * 2];
    int     pos;
    int     gameport_enabled;
} ssi2001_t;

typedef struct entertainer_t {
    uint8_t regs;
} entertainer_t;

static void
ssi2001_update(ssi2001_t *ssi2001)
{
    if (ssi2001->pos >= sound_pos_global)
        return;

    sid_fillbuf(&ssi2001->buffer[ssi2001->pos], sound_pos_global - ssi2001->pos, ssi2001->psid);
    ssi2001->pos = sound_pos_global;
}

static void
ssi2001_get_buffer(int32_t *buffer, int len, void *priv)
{
    ssi2001_t *ssi2001 = (ssi2001_t *) priv;

    ssi2001_update(ssi2001);

    for (int c = 0; c < len * 2; c++)
        buffer[c] += ssi2001->buffer[c >> 1] / 2;

    ssi2001->pos = 0;
}

static uint8_t
ssi2001_read(uint16_t addr, void *priv)
{
    ssi2001_t *ssi2001 = (ssi2001_t *) priv;

    ssi2001_update(ssi2001);

    return sid_read(addr, priv);
}

static void
ssi2001_write(uint16_t addr, uint8_t val, void *priv)
{
    ssi2001_t *ssi2001 = (ssi2001_t *) priv;

    ssi2001_update(ssi2001);
    sid_write(addr, val, priv);
}

void *
ssi2001_init(UNUSED(const device_t *info))
{
    ssi2001_t *ssi2001 = calloc(1, sizeof(ssi2001_t));

    ssi2001->psid = sid_init(device_get_config_int("sid_config"),device_get_config_int("sid_adjustment"));
    sid_reset(ssi2001->psid);
    uint16_t addr             = device_get_config_hex16("base");
    ssi2001->gameport_enabled = device_get_config_int("gameport");
    io_sethandler(addr, 0x0020, ssi2001_read, NULL, NULL, ssi2001_write, NULL, NULL, ssi2001);
    if (ssi2001->gameport_enabled)
        gameport_remap(gameport_add(&gameport_201_device), 0x201);
    sound_add_handler(ssi2001_get_buffer, ssi2001);
    return ssi2001;
}

void
ssi2001_close(void *priv)
{
    ssi2001_t *ssi2001 = (ssi2001_t *) priv;

    sid_close(ssi2001->psid);

    free(ssi2001);
}

static uint8_t
entertainer_read(UNUSED(uint16_t addr), UNUSED(void *priv))
{
    return 0xa5;
}

static void
entertainer_write(UNUSED(uint16_t addr), uint8_t val, void *priv)
{
    entertainer_t *entertainer = (entertainer_t *) priv;
    entertainer->regs = val;
}

void *
entertainer_init(UNUSED(const device_t *info))
{
    ssi2001_t     *ssi2001     = calloc(1, sizeof(ssi2001_t));
    entertainer_t *entertainer = calloc(1, sizeof(entertainer_t));

    ssi2001->psid = sid_init(0, 0.5);
    sid_reset(ssi2001->psid);
    ssi2001->gameport_enabled = device_get_config_int("gameport");
    io_sethandler(0x200, 0x0001, entertainer_read, NULL, NULL, entertainer_write, NULL, NULL, entertainer);
    io_sethandler(0x280, 0x0020, ssi2001_read, NULL, NULL, ssi2001_write, NULL, NULL, ssi2001);
    if (ssi2001->gameport_enabled)
        gameport_remap(gameport_add(&gameport_201_device), 0x201);
    sound_add_handler(ssi2001_get_buffer, ssi2001);
    return ssi2001;
}

void
entertainer_close(void *priv)
{
    ssi2001_t *ssi2001 = (ssi2001_t *) priv;

    sid_close(ssi2001->psid);

    free(ssi2001);
}

static const device_config_t ssi2001_config[] = {
    // clang-format off
    {
        .name           = "base",
        .description    = "Address",
        .type           = CONFIG_HEX16,
        .default_string = NULL,
        .default_int    = 0x280,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
            { .description = "0x280", .value = 0x280 },
            { .description = "0x2A0", .value = 0x2A0 },
            { .description = "0x2C0", .value = 0x2C0 },
            { .description = "0x2E0", .value = 0x2E0 },
            { .description = ""                      }
        },
        .bios           = { { 0 } }
    },
    {
        .name           = "gameport",
        .description    = "Enable Game port",
        .type           = CONFIG_BINARY,
        .default_string = NULL,
        .default_int    = 0,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = { { 0 } },
        .bios           = { { 0 } }
    },
    {
        .name           = "sid_config",
        .description    = "SID Model",
        .type           = CONFIG_HEX16,
        .default_string = NULL,
        .default_int    = 0x000,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {             
		{ .description = "8580", .value = 0x001 },
        { .description = "6581", .value = 0x000 },
		{ .description = ""                      }
		},
        .bios           = { { 0 } }
    },
	{
        .name           = "sid_adjustment",
        .description    = "SID Filter Strength",
        .type           = CONFIG_STRING,
        .default_string = "0.5",
		.default_int    = 0,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {{"0.5"}},
        .bios           = { { 0 } }
    },
    { .name = "", .description = "", .type = CONFIG_END }
// clang-format off
};

static const device_config_t entertainer_config[] = {
    // clang-format off
    {
        .name           = "gameport",
        .description    = "Enable Game port",
        .type           = CONFIG_BINARY,
        .default_string = NULL,
        .default_int    = 0,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = { { 0 } },
        .bios           = { { 0 } }
    },
    { .name = "", .description = "", .type = CONFIG_END }
// clang-format off
};

const device_t ssi2001_device = {
    .name          = "Innovation SSI-2001",
    .internal_name = "ssi2001",
    .flags         = DEVICE_ISA,
    .local         = 0,
    .init          = ssi2001_init,
    .close         = ssi2001_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = NULL,
    .force_redraw  = NULL,
    .config        = ssi2001_config
};

const device_t entertainer_device = {
    .name          = "The Entertainer",
    .internal_name = "Entertainer",
    .flags         = DEVICE_ISA,
    .local         = 1,
    .init          = entertainer_init,
    .close         = entertainer_close,
    .reset         = NULL,
    .available     = NULL,
    .speed_changed = NULL,
    .force_redraw  = NULL,
    .config        = entertainer_config
};
