/* -*- mode: c; c-basic-offset: 4 -*- */

/* Copyright (C) 2022 Alexander Chernov <cher@ejudge.ru> */

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

#include "mysql_conn.h"
#include "telegram_pbs.h"
#include "telegram_token.h"

#include "ejudge/common_plugin.h"
#include "../common-mysql/common_mysql.h"
#include "ejudge/xalloc.h"
#include "ejudge/errlog.h"

static struct generic_conn *
free_func(struct generic_conn *gc)
{
    if (gc) {
        struct mysql_conn *conn = (struct mysql_conn *) gc;
        free(conn->b.database);
        free(conn->b.host);
        free(conn->b.table_prefix);
        free(conn->b.user);
        free(conn->b.password);
        free(conn);
    }
    return NULL;
}

static int
prepare_func(
        struct generic_conn *gc,
        const struct ejudge_cfg *config,
        struct xml_tree *tree)
{
    struct mysql_conn *conn = (struct mysql_conn *) gc;
    const struct common_loaded_plugin *mplg;

    // load common_mysql plugin
    if (!(mplg = plugin_load_external(0, "common", "mysql", config))) {
        err("cannot load common_mysql plugin");
        return -1;
    }
    conn->mi = (struct common_mysql_iface*) mplg->iface;
    conn->md = (struct common_mysql_state*) mplg->data;

    return 0;
}

static const char create_query_1[] =
"CREATE TABLE %stelegram_bots (\n"
"    id CHAR(64) NOT NULL PRIMARY KEY,\n"
"    update_id INT(18) NOT NULL DEFAULT 0\n"
") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;\n";

static const char create_query_2[] =
"CREATE TABLE %stelegram_tokens (\n"
"    id INT(18) NOT NULL PRIMARY KEY AUTO_INCREMENT,\n"
"    bot_id CHAR(64) NOT NULL,\n"
"    user_id INT UNSIGNED NOT NULL,\n"
"    user_login VARCHAR(64) DEFAULT NULL,\n"
"    user_name VARCHAR(512) DEFAULT NULL,\n"
"    token CHAR(64) NOT NULL,\n"
"    contest_id INT NOT NULL DEFAULT 0,\n"
"    contest_name VARCHAR(512) DEFAULT NULL,\n"
"    locale_id INT NOT NULL DEFAULT 0,\n"
"    expiry_time DATETIME NOT NULL,\n"
"    KEY tt_bot_id_k(bot_id),\n"
"    KEY tt_contest_id_k(contest_id),\n"
"    KEY tt_contest_user_k(contest_id,user_id),\n"
"    UNIQUE KEY tt_token_k(token),\n"
"    FOREIGN KEY tt_user_id_fk(user_id) REFERENCES %slogins(user_id)\n"
") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;\n";

static const char create_query_3[] =
"CREATE TABLE %stelegram_users (\n"
"    id INT(18) NOT NULL PRIMARY KEY,\n"
"    username VARCHAR(512) DEFAULT NULL,\n"
"    first_name VARCHAR(512) DEFAULT NULL,\n"
"    last_name VARCHAR(512) DEFAULT NULL\n"
") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;\n";

static const char create_query_4[] =
"CREATE TABLE %stelegram_users (\n"
"    id INT(18) NOT NULL PRIMARY KEY,\n"
"    username VARCHAR(512) DEFAULT NULL,\n"
"    first_name VARCHAR(512) DEFAULT NULL,\n"
"    last_name VARCHAR(512) DEFAULT NULL\n"
") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;\n";

static const char create_query_5[] =
"CREATE TABLE %stelegram_chats (\n"
"    id INT(18) NOT NULL PRIMARY KEY,\n"
"    chat_type VARCHAR(64) DEFAULT NULL,\n"
"    title VARCHAR(512) DEFAULT NULL,\n"
"    username VARCHAR(512) DEFAULT NULL,\n"
"    first_name VARCHAR(512) DEFAULT NULL,\n"
"    last_name VARCHAR(512) DEFAULT NULL\n"
") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;\n";

static const char create_query_6[] =
"CREATE TABLE %stelegram_chat_states (\n"
"    id INT(18) NOT NULL PRIMARY KEY,\n"
"    command VARCHAR(64) DEFAULT NULL,\n"
"    token VARCHAR(64) DEFAULT NULL,\n"
"    state INT NOT NULL DEFAULT 0,\n"
"    review_flag INT NOT NULL DEFAULT 0,\n"
"    reply_flag INT NOT NULL DEFAULT 0\n"
") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;\n";

static const char create_query_7[] =
"CREATE TABLE %stelegram_subscriptions (\n"
"    id CHAR(64) NOT NULL PRIMARY KEY,\n"
"    bot_id CHAR(64) NOT NULL,\n"
"    user_id INT UNSIGNED NOT NULL,\n"
"    contest_id INT NOT NULL DEFAULT 0,\n"
"    review_flag INT NOT NULL DEFAULT 0,\n"
"    reply_flag INT NOT NULL DEFAULT 0,\n"
"    chat_id INT(18) NOT NULL DEFAULT 0,\n"
"    KEY ts_bot_id_k(bot_id),\n"
"    KEY ts_contest_id_k(contest_id),\n"
"    FOREIGN KEY ts_user_id_fk(user_id) REFERENCES %slogins(user_id)\n"
") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;\n";

static int
create_database(
        struct mysql_conn *conn)
{
    struct common_mysql_iface *mi = conn->mi;
    struct common_mysql_state *md = conn->md;

    if (mi->simple_fquery(md, create_query_1,
                          md->table_prefix) < 0)
        db_error_fail(md);
    if (mi->simple_fquery(md, create_query_2,
                          md->table_prefix,
                          md->table_prefix) < 0)
        db_error_fail(md);
    if (mi->simple_fquery(md, create_query_3,
                          md->table_prefix) < 0)
        db_error_fail(md);
    if (mi->simple_fquery(md, create_query_4,
                          md->table_prefix) < 0)
        db_error_fail(md);
    if (mi->simple_fquery(md, create_query_5,
                          md->table_prefix) < 0)
        db_error_fail(md);
    if (mi->simple_fquery(md, create_query_6,
                          md->table_prefix) < 0)
        db_error_fail(md);
    if (mi->simple_fquery(md, create_query_7,
                          md->table_prefix,
                          md->table_prefix) < 0)
        db_error_fail(md);

    return 0;

fail:
    return -1;
}

static int
check_database(
        struct mysql_conn *conn)
{
    int telegram_version = 0;
    struct common_mysql_iface *mi = conn->mi;
    struct common_mysql_state *md = conn->md;

    if (mi->connect(md) < 0)
        return -1;
    if (mi->fquery(md, 1, "SELECT config_val FROM %sconfig WHERE config_key = 'telegram_version' ;", md->table_prefix) < 0) {
        err("probably the database is not created, please, create it");
        return -1;
    }
    if (md->row_count > 1) abort();
    if (!md->row_count) return create_database(conn);
    if (mi->next_row(md) < 0) db_error_fail(md);
    if (!md->row[0] || mi->parse_int(md, md->row[0], &telegram_version) < 0)
        db_error_inv_value_fail(md, "config_val");
    mi->free_res(md);

    if (telegram_version < 1) {
        err("telegram_version == %d is not supported", telegram_version);
        goto fail;
    }

    while (telegram_version >= 0) {
        switch (telegram_version) {
        default:
            telegram_version = -1;
            break;
        }
        if (telegram_version >= 0) {
            ++telegram_version;
            if (mi->simple_fquery(md, "UPDATE %sconfig SET config_val = '%d' WHERE config_key = 'telegram_version' ;", md->table_prefix, telegram_version) < 0)
                return -1;
        }
    }

    return 0;

fail:
    return -1;
}

static int
open_func(struct generic_conn *gc)
{
    struct mysql_conn *conn = (struct mysql_conn *) gc;

    if (!conn->is_db_checked) {
        if (check_database(conn) < 0) {
            return -1;
        }
        conn->is_db_checked = 1;
    }

    return 0;
}

struct telegram_pbs_internal
{
    unsigned char *id;
    long long update_id;
};
enum { TELEGRAM_PBS_ROW_WIDTH = 2 };
#define TELEGRAM_PBS_OFFSET(f) XOFFSET(struct telegram_pbs_internal, f)
static const struct common_mysql_parse_spec telegram_pbs_spec[TELEGRAM_PBS_ROW_WIDTH] =
{
    { 1, 's', "id", TELEGRAM_PBS_OFFSET(id), 0 },
    { 0, 'q', "update_id", TELEGRAM_PBS_OFFSET(update_id), 0 },
};

static struct telegram_pbs *
pbs_fetch_func(
        struct generic_conn *gc,
        const unsigned char *bot_id)
{
    if (gc->vt->open(gc) < 0) return NULL;

    struct mysql_conn *conn = (struct mysql_conn *) gc;
    struct common_mysql_iface *mi = conn->mi;
    struct common_mysql_state *md = conn->md;
    char *cmd_s = NULL;
    size_t cmd_z = 0;
    FILE *cmd_f = NULL;
    struct telegram_pbs_internal tpi = {};
    struct telegram_pbs *tp = NULL;

    cmd_f = open_memstream(&cmd_s, &cmd_z);
    fprintf(cmd_f, "SELECT * FROM %stelegram_bots WHERE id = '",
            md->table_prefix);
    mi->write_escaped_string(md, cmd_f, NULL, bot_id);
    fprintf(cmd_f, "'");
    fclose(cmd_f); cmd_f = NULL;
    if (mi->query(md, cmd_s, cmd_z, TELEGRAM_PBS_ROW_WIDTH) < 0)
        db_error_fail(md);
    free(cmd_s); cmd_s = NULL; cmd_z = 0;
    if (md->row_count > 0) {
        if (mi->next_row(md) < 0) db_error_fail(md);
        if (mi->parse_spec(md, -1, md->row, md->lengths, TELEGRAM_PBS_ROW_WIDTH, telegram_pbs_spec, &tpi) < 0) goto fail;
        XCALLOC(tp, 1);
        tp->_id = tpi.id; tpi.id = NULL;
        tp->update_id = tpi.update_id;
        return tp;
    }

    tp = telegram_pbs_create(bot_id);
    gc->vt->pbs_save(gc, tp);

    return tp;

fail:
    telegram_pbs_free(tp);
    if (cmd_f) fclose(cmd_f);
    free(cmd_s);
    free(tpi.id);
    return NULL;
}

static int
pbs_save_func(
        struct generic_conn *gc,
        const struct telegram_pbs *pbs)
{
    if (gc->vt->open(gc) < 0) return -1;

    struct mysql_conn *conn = (struct mysql_conn *) gc;
    struct common_mysql_iface *mi = conn->mi;
    struct common_mysql_state *md = conn->md;
    char *cmd_s = NULL;
    size_t cmd_z = 0;
    FILE *cmd_f = NULL;
    struct telegram_pbs_internal tpi = {};

    tpi.id = pbs->_id;
    tpi.update_id = pbs->update_id;
    cmd_f = open_memstream(&cmd_s, &cmd_z);
    fprintf(cmd_f, "INSERT INTO %stelegram_bots SET ", md->table_prefix);
    mi->unparse_spec_3(md, cmd_f, TELEGRAM_PBS_ROW_WIDTH,
                       telegram_pbs_spec, 0, &tpi);
    fprintf(cmd_f, " ON DUPLICATE KEY UPDATE ");
    mi->unparse_spec_3(md, cmd_f, TELEGRAM_PBS_ROW_WIDTH,
                       telegram_pbs_spec, 1ULL, &tpi);
    fprintf(cmd_f, ";");
    fclose(cmd_f); cmd_f = NULL;
    if (mi->simple_query(md, cmd_s, cmd_z) < 0) goto fail;
    free(cmd_s); cmd_s = NULL; cmd_z = 0;
    return 0;

fail:
    if (cmd_f) fclose(cmd_f);
    free(cmd_s);
    return -1;
}

struct telegram_token_internal
{
    long long id;
    unsigned char *bot_id;
    int user_id;
    unsigned char *user_login;
    unsigned char *user_name;
    unsigned char *token;
    int contest_id;
    unsigned char *contest_name;
    int locale_id;
    time_t expiry_time;
};
enum { TELEGRAM_TOKEN_ROW_WIDTH = 10 };
#define TELEGRAM_TOKEN_OFFSET(f) XOFFSET(struct telegram_token_internal, f)
static const struct common_mysql_parse_spec telegram_token_spec[TELEGRAM_TOKEN_ROW_WIDTH] =
{
    { 0, 'q', "id", TELEGRAM_TOKEN_OFFSET(id), 0 },
    { 1, 's', "bot_id", TELEGRAM_TOKEN_OFFSET(bot_id), 0 },
    { 0, 'd', "user_id", TELEGRAM_TOKEN_OFFSET(user_id), 0 },
    { 1, 's', "user_login", TELEGRAM_TOKEN_OFFSET(user_login), 0 },
    { 1, 's', "user_name", TELEGRAM_TOKEN_OFFSET(user_name), 0 },
    { 1, 's', "token", TELEGRAM_TOKEN_OFFSET(token), 0 },
    { 0, 'd', "contest_id", TELEGRAM_TOKEN_OFFSET(contest_id), 0 },
    { 1, 's', "contest_name", TELEGRAM_TOKEN_OFFSET(contest_name), 0 },
    { 0, 'd', "locale_id", TELEGRAM_TOKEN_OFFSET(locale_id), 0 },
    { 1, 't', "expiry_time", TELEGRAM_TOKEN_OFFSET(expiry_time), 0 },
};

static int
token_fetch_func(
        struct generic_conn *gc,
        const unsigned char *token_str,
        struct telegram_token **p_token)
{
    if (gc->vt->open(gc) < 0) return -1;

    struct mysql_conn *conn = (struct mysql_conn *) gc;
    struct common_mysql_iface *mi = conn->mi;
    struct common_mysql_state *md = conn->md;
    char *cmd_s = NULL;
    size_t cmd_z = 0;
    FILE *cmd_f = NULL;
    struct telegram_token_internal tti = {};
    struct telegram_token *tt = NULL;

    cmd_f = open_memstream(&cmd_s, &cmd_z);
    fprintf(cmd_f, "SELECT * FROM %stelegram_tokens WHERE token = '",
            md->table_prefix);
    mi->write_escaped_string(md, cmd_f, NULL, token_str);
    fprintf(cmd_f, "'");
    fclose(cmd_f); cmd_f = NULL;
    if (mi->query(md, cmd_s, cmd_z, TELEGRAM_TOKEN_ROW_WIDTH) < 0)
        db_error_fail(md);
    free(cmd_s); cmd_s = NULL; cmd_z = 0;
    if (md->row_count == 1) {
        if (mi->next_row(md) < 0) db_error_fail(md);
        if (mi->parse_spec(md, -1, md->row, md->lengths, TELEGRAM_TOKEN_ROW_WIDTH, telegram_token_spec, &tti) < 0) goto fail;
        XCALLOC(tt, 1);
        tt->bot_id = tti.bot_id; tti.bot_id = NULL;
        tt->user_id = tti.user_id;
        tt->user_login = tti.user_login; tti.user_login = NULL;
        tt->user_name = tti.user_name; tti.user_name = NULL;
        tt->token = tti.token; tti.token = NULL;
        tt->contest_id = tti.contest_id;
        tt->contest_name = tti.contest_name; tti.contest_name = NULL;
        tt->locale_id = tti.locale_id;
        tt->expiry_time = tti.expiry_time;
        *p_token = tt;
        return 1;
    }

    return 0;

fail:
    telegram_token_free(tt);
    free(tti.bot_id);
    free(tti.user_login);
    free(tti.user_name);
    free(tti.token);
    free(tti.contest_name);
    if (cmd_f) fclose(cmd_f);
    free(cmd_s);
    return -1;
}

static int
token_save_func(
        struct generic_conn *gc,
        const struct telegram_token *token)
{
    if (gc->vt->open(gc) < 0) return -1;

    struct mysql_conn *conn = (struct mysql_conn *) gc;
    struct common_mysql_iface *mi = conn->mi;
    struct common_mysql_state *md = conn->md;
    char *cmd_s = NULL;
    size_t cmd_z = 0;
    FILE *cmd_f = NULL;
    struct telegram_token_internal tti = {};

    tti.bot_id = token->bot_id;
    tti.user_id = token->user_id;
    tti.user_login = token->user_login;
    tti.user_name = token->user_name;
    tti.token = token->token;
    tti.contest_id = token->contest_id;
    tti.contest_name = token->contest_name;
    tti.locale_id = token->locale_id;
    tti.expiry_time = token->expiry_time;

    cmd_f = open_memstream(&cmd_s, &cmd_z);
    fprintf(cmd_f, "INSERT INTO %stelegram_tokens VALUES (DEFAULT,",
            md->table_prefix);
    mi->unparse_spec_2(md, cmd_f, TELEGRAM_TOKEN_ROW_WIDTH,
                       telegram_token_spec, 1ULL, &tti);
    fprintf(cmd_f, ");");
    fclose(cmd_f); cmd_f = NULL;
    if (mi->simple_query(md, cmd_s, cmd_z) < 0) goto fail;
    free(cmd_s); cmd_s = NULL; cmd_z = 0;
    return 0;

fail:
    if (cmd_f) fclose(cmd_f);
    free(cmd_s);
    return -1;
}

static void
token_remove_func(
        struct generic_conn *gc,
        const unsigned char *token)
{
    if (gc->vt->open(gc) < 0) return;

    struct mysql_conn *conn = (struct mysql_conn *) gc;
    struct common_mysql_iface *mi = conn->mi;
    struct common_mysql_state *md = conn->md;
    char *cmd_s = NULL;
    size_t cmd_z = 0;
    FILE *cmd_f = NULL;

    cmd_f = open_memstream(&cmd_s, &cmd_z);
    fprintf(cmd_f, "DELETE FROM %stelegram_tokens WHERE token = '",
            md->table_prefix);
    mi->write_escaped_string(md, cmd_f, NULL, token);
    fprintf(cmd_f, "'");
    fclose(cmd_f); cmd_f = NULL;
    if (mi->simple_query(md, cmd_s, cmd_z) < 0)
        db_error_fail(md);
    free(cmd_s); cmd_s = NULL; cmd_z = 0;
    return;

fail:
    if (cmd_f) fclose(cmd_f);
    free(cmd_s);
}

static void
token_remove_expired_func(
        struct generic_conn *gc,
        time_t current_time)
{
    if (gc->vt->open(gc) < 0) return;

    struct mysql_conn *conn = (struct mysql_conn *) gc;
    struct common_mysql_iface *mi = conn->mi;
    struct common_mysql_state *md = conn->md;
    char *cmd_s = NULL;
    size_t cmd_z = 0;
    FILE *cmd_f = NULL;

    cmd_f = open_memstream(&cmd_s, &cmd_z);
    fprintf(cmd_f, "DELETE FROM %stelegram_tokens WHERE expiry_time = '",
            md->table_prefix);
    mi->write_timestamp(md, cmd_f, NULL, current_time);
    fprintf(cmd_f, "'");
    fclose(cmd_f); cmd_f = NULL;
    if (mi->simple_query(md, cmd_s, cmd_z) < 0)
        db_error_fail(md);
    free(cmd_s); cmd_s = NULL; cmd_z = 0;
    return;

fail:
    if (cmd_f) fclose(cmd_f);
    free(cmd_s);
}

static struct generic_conn_iface mysql_iface =
{
    free_func,
    prepare_func,
    open_func,
    NULL,
    pbs_fetch_func,
    pbs_save_func,
    token_fetch_func,
    token_save_func,
    token_remove_func,
    token_remove_expired_func,
};

struct generic_conn *
mysql_conn_create(void)
{
    struct mysql_conn *conn = NULL;
    XCALLOC(conn, 1);
    conn->b.vt = &mysql_iface;
    return &conn->b;
}

