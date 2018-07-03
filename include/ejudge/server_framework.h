/* -*- c -*- */

#ifndef __SERVER_FRAMEWORK_H__
#define __SERVER_FRAMEWORK_H__

/* Copyright (C) 2006-2018 Alexander Chernov <cher@ejudge.ru> */

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

#include <unistd.h>
#include <time.h>

enum
{
  STATE_READ_CREDS,
  STATE_READ_FDS,
  STATE_READ_LEN,
  STATE_READ_DATA,
  STATE_READ_READY,
  STATE_WRITE,
  STATE_WRITECLOSE,
  STATE_DISCONNECT,
};

enum
{
  WS_STATE_INITIAL,
  WS_STATE_INITIAL_REPLY,
  WS_STATE_HTTP_ERROR,
  WS_STATE_ACTIVE,

  WS_STATE_DISCONNECT,
};

struct ws_frame
{
  struct ws_frame *prev;
  struct ws_frame *next;

  unsigned char *data;
  int size;
  int fragments;
  unsigned char hdr[2];
};

struct client_state;

struct client_state_operations
{
  void (*destroy)(struct client_state *);

  int (*get_peer_uid)(const struct client_state *);
  int (*get_contest_id)(const struct client_state *);

  void (*set_destroy_callback)(
        struct client_state *p,
        int cnts_id,
        void (*destroy_callback)(struct client_state*));
};

struct client_state
{
  const struct client_state_operations *ops;
  struct client_state *prev;
  struct client_state *next;

  int id;
  int fd;
  int state;

  int peer_pid;
  int peer_uid;
  int peer_gid;

  int client_fds[2];

  int expected_len;
  int read_len;
  unsigned char *read_buf;

  int write_len;
  int written;
  unsigned char *write_buf;

  int contest_id;
  void (*destroy_callback)(struct client_state*);
};

struct ws_client_state
{
  struct ws_client_state *prev;
  struct ws_client_state *next;

  struct ws_frame *frame_first;
  struct ws_frame *frame_last;

  unsigned char *remote_host;
  unsigned char *read_buf;
  unsigned char *write_buf;

  unsigned char *uri;
  unsigned char *host;
  unsigned char *user_agent;
  unsigned char *accept_encoding;
  unsigned char *origin;
  
  long long last_read_time_us;
  long long last_write_time_us;

  int id;
  int fd;

  int remote_port;
  int read_reserved;
  int read_expected;
  int read_size;

  int write_reserved;
  int write_size;

  unsigned char ssl_flag;
  unsigned char state;
  unsigned char in_close_state; // 0 - input active, 1 - close received, 2 - EOF event read
  unsigned char out_close_state; // 0 - output active, 1 - close in the output queue, 2 - close on the wire
  unsigned char hdr_flag;

  unsigned char hdr_expected;
  unsigned char hdr_size;
  unsigned char hdr_buf[16];
};

struct server_framework_state;
struct new_server_prot_packet;

struct server_framework_params
{
  int daemon_mode_flag;
  int restart_mode_flag;
  int force_socket_flag;
  unsigned char *program_name;
  unsigned char *socket_path;
  unsigned char *log_path;
  int select_timeout;

  void *user_data;

  void (*startup_error)(const char *, ...);
  void (*handle_packet)(struct server_framework_state *,
                        struct client_state *,
                        size_t,
                        const struct new_server_prot_packet *);
  struct client_state *(*alloc_state)(struct server_framework_state *);
  void (*cleanup_client)(struct server_framework_state *,
                         struct client_state *);
  void (*free_memory)(struct server_framework_state *, void *);
  int  (*loop_start)(struct server_framework_state *);
  void (*post_select)(struct server_framework_state *);

  // WebSocket port, if > 0, then the server listens for websocket incoming connections
  int ws_port;

  struct ws_client_state *(*ws_alloc_state)(
        struct server_framework_state *);

  void (*ws_handle_packet)(
        struct server_framework_state *,
        struct ws_client_state *,
        int opcode,
        const unsigned char *data,
        size_t);

  void (*ws_cleanup)(
        struct server_framework_state *,
        struct ws_client_state *);
};

struct server_framework_state *nsf_init(struct server_framework_params *params, void *data, time_t server_start_time);
int  nsf_prepare(struct server_framework_state *state);
void nsf_cleanup(struct server_framework_state *state);

void nsf_main_loop(struct server_framework_state *state);

void nsf_enqueue_reply(struct server_framework_state *state,
                       struct client_state *p, ej_size_t len, void const *msg);
void nsf_send_reply(struct server_framework_state *state,
                    struct client_state *p, int answer);
void nsf_new_autoclose(struct server_framework_state *state,
                       struct client_state *p, void *write_buf,
                       size_t write_len);
void nsf_close_client_fds(struct client_state *p);
struct client_state * nsf_get_client_by_id(struct server_framework_state *,
                                           int id);

enum
{
  NSF_READ = 1, NSF_WRITE = 2, NSF_RW = 3
};
struct server_framework_watch
{
  int fd;
  int mode;
  void (*callback)(struct server_framework_state *,
                   struct server_framework_watch *,
                   int event);
  void *user;
};

int nsf_add_watch(struct server_framework_state *,
                  struct server_framework_watch*);
int nsf_remove_watch(struct server_framework_state *, int);
int nsf_is_restart_requested(struct server_framework_state *);

void nsf_err_bad_packet_length(struct server_framework_state *,
                               struct client_state *, size_t, size_t);
void nsf_err_invalid_command(struct server_framework_state *,
                             struct client_state *, int);
void nsf_err_packet_too_small(struct server_framework_state *,
                              struct client_state *, size_t, size_t);
void nsf_err_protocol_error(struct server_framework_state *,
                            struct client_state *);

struct server_framework_job;
struct server_framework_job_funcs
{
  void (*destroy)(struct server_framework_job *);
  int  (*run)(struct server_framework_job *, int *p_tick_value, int max_value);
  unsigned char * (*get_status)(struct server_framework_job *);
};

struct server_framework_job
{
  const struct server_framework_job_funcs *vt;
  struct server_framework_job *prev, *next;
  int id;
  int prio;
  int contest_id;
  time_t start_time;
  unsigned char *title;
};

void
nsf_add_job(
        struct server_framework_state *state,
        struct server_framework_job *job);
void
nsf_remove_job(
        struct server_framework_state *state,
        struct server_framework_job *job);
struct server_framework_job *
nsf_get_first_job(
        struct server_framework_state *state);
int
nsf_get_job_count(
        struct server_framework_state *state);

time_t
nsf_get_server_start_time(
        struct server_framework_state *state);

int
nsf_ws_append_reply_frame(
        struct ws_client_state *p,
        int opcode,
        const unsigned char *data,
        int size);

#endif /* __SERVER_FRAMEWORK_H__ */
