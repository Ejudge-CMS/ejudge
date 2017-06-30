/* -*- mode: c -*- */

/* Copyright (C) 2017 Alexander Chernov <cher@ejudge.ru> */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ejudge/avatar_plugin.h"
#include "ejudge/ejudge_cfg.h"
#include "ejudge/contests.h"
#include "ejudge/serve_state.h"

#include "ejudge/xalloc.h"
#include "ejudge/errlog.h"

#include <string.h>

#define DEFAULT_AVATAR_PLUGIN "mongo"

#define AVATAR_PLUGIN_TYPE "avatar"

struct avatar_loaded_plugin *
avatar_plugin_get(
        struct serve_state *cs,
        const struct contest_desc *cnts,
        const struct ejudge_cfg *config,
        const unsigned char *plugin_name)
{
    if (!plugin_name && cnts) plugin_name = cnts->avatar_plugin;
    if (!plugin_name && config) plugin_name = config->default_avatar_plugin;
    if (!plugin_name) plugin_name = DEFAULT_AVATAR_PLUGIN;

    if (cs->main_avatar_plugin) {
        if (!strcmp(cs->main_avatar_plugin->name, plugin_name)) {
            return cs->main_avatar_plugin;
        }
        // FIXME: support multiple plugins per contest
        err("default avatar plugin is '%s', but plugin '%s' requested for load", cs->main_avatar_plugin->name, plugin_name);
        return NULL;
    }

    const struct ejudge_plugin *plg = NULL;
    for (const struct xml_tree *p = config->plugin_list; p; p = p->right) {
        plg = (const struct ejudge_plugin*) p;
        if (plg->load_flag && !strcmp(plg->type, AVATAR_PLUGIN_TYPE) && !strcmp(plg->name, plugin_name))
            break;
        plg = NULL;
    }
    if (!plg) {
        err("avatar plugin '%s' is not registered", plugin_name);
        return NULL;
    }

    const struct common_loaded_plugin *loaded_plugin = plugin_load_external(plg->path, plg->type, plg->name, config);
    if (!loaded_plugin) {
        err("cannot load plugin %s, %s", plg->type, plg->name);
        return NULL;
    }

    struct avatar_loaded_plugin *avt = NULL;
    XCALLOC(avt, 1);
    avt->common = loaded_plugin;
    avt->name = xstrdup(plugin_name);
    avt->iface = (struct avatar_plugin_iface*) loaded_plugin->iface;
    avt->data = (struct avatar_plugin_data*) loaded_plugin->data;

    cs->main_avatar_plugin = avt;
    return avt;
}

struct avatar_loaded_plugin *
avatar_plugin_destroy(struct avatar_loaded_plugin *plugin)
{
    if (plugin) {
        xfree(plugin->name);
        xfree(plugin);
    }
    return NULL;
}


/*
 * Local variables:
 *  c-basic-offset: 4
 * End:
 */
