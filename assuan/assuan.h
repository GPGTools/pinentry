/* assuan.c - Definitions for the Assuna protocol
 *	Copyright (C) 2001, 2002 Free Software Foundation, Inc.
 *
 * This file is part of GnuPG.
 *
 * GnuPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GnuPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef ASSUAN_H
#define ASSUAN_H

#include <stdio.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" { 
#if 0
 }
#endif
#endif

/* 5 is pinentry.  */
#define ASSUAN_ERROR(code) ((5 << 24)  | code)

typedef enum {
  ASSUAN_No_Error = 0,
  ASSUAN_General_Error = ASSUAN_ERROR (257),
  ASSUAN_Out_Of_Core = ASSUAN_ERROR (86 | (1 << 15)),
  ASSUAN_Invalid_Value = ASSUAN_ERROR (261),
  ASSUAN_Timeout = ASSUAN_ERROR (62),
  ASSUAN_Read_Error = ASSUAN_ERROR (270),  /* Not 100%, but sufficient here. */
  ASSUAN_Write_Error = ASSUAN_ERROR (271), /* Not 100%, but sufficient here. */
  ASSUAN_Problem_Starting_Server = ASSUAN_ERROR (269),
  ASSUAN_Not_A_Server = ASSUAN_ERROR (267),
  ASSUAN_Not_A_Client = ASSUAN_ERROR (268),
  ASSUAN_Nested_Commands = ASSUAN_ERROR (264),
  ASSUAN_Invalid_Response = ASSUAN_ERROR (260),
  ASSUAN_No_Data_Callback = ASSUAN_ERROR (265),
  ASSUAN_No_Inquire_Callback = ASSUAN_ERROR (266),
  ASSUAN_Connect_Failed = ASSUAN_ERROR (259),
  ASSUAN_Accept_Failed = ASSUAN_ERROR (258),

  /* error codes above 99 are meant as status codes */
  ASSUAN_Not_Implemented = ASSUAN_ERROR (69),
  ASSUAN_Server_Fault    = ASSUAN_ERROR (80),
  ASSUAN_Unknown_Command = ASSUAN_ERROR (275),
  ASSUAN_Syntax_Error    = ASSUAN_ERROR (276),
  ASSUAN_Parameter_Conflict = ASSUAN_ERROR (280),
  ASSUAN_Line_Too_Long = ASSUAN_ERROR (263),
  ASSUAN_Line_Not_Terminated = ASSUAN_ERROR (262),
  ASSUAN_Canceled = ASSUAN_ERROR (99),
  ASSUAN_Invalid_Option = ASSUAN_ERROR (174), /* GPG_ERR_UNKNOWN_OPTION */
  ASSUAN_Locale_Problem = ASSUAN_ERROR (166),
  ASSUAN_Not_Confirmed = ASSUAN_ERROR (114),

} assuan_error_t;

#define ASSUAN_Parameter_Error ASSUAN_Parameter_Conflict


typedef assuan_error_t AssuanError; /* Deprecated. */

/* This is a list of pre-registered ASSUAN commands */
typedef enum {
  ASSUAN_CMD_NOP = 0,
  ASSUAN_CMD_CANCEL,    /* cancel the current request */
  ASSUAN_CMD_BYE,
  ASSUAN_CMD_AUTH,
  ASSUAN_CMD_RESET,
  ASSUAN_CMD_OPTION,
  ASSUAN_CMD_DATA,
  ASSUAN_CMD_END,
  ASSUAN_CMD_INPUT,
  ASSUAN_CMD_OUTPUT,

  ASSUAN_CMD_USER = 256  /* Other commands should be used with this offset*/
} AssuanCommand;

#define ASSUAN_LINELENGTH 1002 /* 1000 + [CR,]LF */

struct assuan_context_s;
typedef struct assuan_context_s *assuan_context_t; 
typedef struct assuan_context_s *ASSUAN_CONTEXT; /* Deprecated.  */

/*-- assuan-handler.c --*/
int assuan_register_command (ASSUAN_CONTEXT ctx,
                             int cmd_id, const char *cmd_string,
                             int (*handler)(ASSUAN_CONTEXT, char *));
int assuan_register_bye_notify (ASSUAN_CONTEXT ctx,
                                void (*fnc)(ASSUAN_CONTEXT));
int assuan_register_reset_notify (ASSUAN_CONTEXT ctx,
                                  void (*fnc)(ASSUAN_CONTEXT));
int assuan_register_cancel_notify (ASSUAN_CONTEXT ctx,
                                   void (*fnc)(ASSUAN_CONTEXT));
int assuan_register_input_notify (ASSUAN_CONTEXT ctx,
                                  void (*fnc)(ASSUAN_CONTEXT, const char *));
int assuan_register_output_notify (ASSUAN_CONTEXT ctx,
                                  void (*fnc)(ASSUAN_CONTEXT, const char *));

int assuan_register_option_handler (ASSUAN_CONTEXT ctx,
                                    int (*fnc)(ASSUAN_CONTEXT,
                                               const char*, const char*));

int assuan_process (ASSUAN_CONTEXT ctx);
int assuan_process_next (ASSUAN_CONTEXT ctx);
int assuan_get_active_fds (ASSUAN_CONTEXT ctx, int what,
                           int *fdarray, int fdarraysize);


AssuanError assuan_set_okay_line (ASSUAN_CONTEXT ctx, const char *line);
void assuan_write_status (ASSUAN_CONTEXT ctx,
                          const char *keyword, const char *text);


/*-- assuan-listen.c --*/
AssuanError assuan_set_hello_line (ASSUAN_CONTEXT ctx, const char *line);
AssuanError assuan_accept (ASSUAN_CONTEXT ctx);
int assuan_get_input_fd (ASSUAN_CONTEXT ctx);
int assuan_get_output_fd (ASSUAN_CONTEXT ctx);
AssuanError assuan_close_input_fd (ASSUAN_CONTEXT ctx);
AssuanError assuan_close_output_fd (ASSUAN_CONTEXT ctx);


/*-- assuan-pipe-server.c --*/
int assuan_init_pipe_server (ASSUAN_CONTEXT *r_ctx, int filedes[2]);
void assuan_deinit_server (ASSUAN_CONTEXT ctx);

/*-- assuan-socket-server.c --*/
int assuan_init_socket_server (ASSUAN_CONTEXT *r_ctx, int listen_fd);


/*-- assuan-pipe-connect.c --*/
AssuanError assuan_pipe_connect (ASSUAN_CONTEXT *ctx, const char *name,
                                 char *const argv[], int *fd_child_list);
/*-- assuan-socket-connect.c --*/
AssuanError assuan_socket_connect (ASSUAN_CONTEXT *ctx, const char *name,
                                   pid_t server_pid);

/*-- assuan-connect.c --*/
void assuan_disconnect (ASSUAN_CONTEXT ctx);
pid_t assuan_get_pid (ASSUAN_CONTEXT ctx);

/*-- assuan-client.c --*/
AssuanError 
assuan_transact (ASSUAN_CONTEXT ctx,
                 const char *command,
                 AssuanError (*data_cb)(void *, const void *, size_t),
                 void *data_cb_arg,
                 AssuanError (*inquire_cb)(void*, const char *),
                 void *inquire_cb_arg,
                 AssuanError (*status_cb)(void*, const char *),
                 void *status_cb_arg);


/*-- assuan-inquire.c --*/
AssuanError assuan_inquire (ASSUAN_CONTEXT ctx, const char *keyword,
                            char **r_buffer, size_t *r_length, size_t maxlen);

/*-- assuan-buffer.c --*/
AssuanError assuan_read_line (ASSUAN_CONTEXT ctx,
                              char **line, size_t *linelen);
int assuan_pending_line (ASSUAN_CONTEXT ctx);
AssuanError assuan_write_line (ASSUAN_CONTEXT ctx, const char *line );
AssuanError assuan_send_data (ASSUAN_CONTEXT ctx,
                              const void *buffer, size_t length);


/*-- assuan-util.c --*/
void assuan_set_malloc_hooks ( void *(*new_alloc_func)(size_t n),
                               void *(*new_realloc_func)(void *p, size_t n),
                               void (*new_free_func)(void*) );
void assuan_set_log_stream (ASSUAN_CONTEXT ctx, FILE *fp);
int assuan_set_error (ASSUAN_CONTEXT ctx, int err, const char *text);
void assuan_set_pointer (ASSUAN_CONTEXT ctx, void *pointer);
void *assuan_get_pointer (ASSUAN_CONTEXT ctx);

void assuan_begin_confidential (ASSUAN_CONTEXT ctx);
void assuan_end_confidential (ASSUAN_CONTEXT ctx);

/*-- assuan-errors.c (built) --*/
const char *assuan_strerror (AssuanError err);


#ifdef __cplusplus
}
#endif
#endif /*ASSUAN_H*/
