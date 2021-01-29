/* pinentry-efl.c
   Copyright (C) 2017 Obsidian-Studios, Inc.
     Author William L. Thomson Jr. <wlt@o-sinc.com>

   Based on pinentry-gtk2.c
   Copyright (C) 1999 Robert Bihlmeyer <robbe@orcus.priv.at>
   Copyright (C) 2001, 2002, 2007, 2015 g10 Code GmbH
   Copyright (C) 2004 by Albrecht Dreß <albrecht.dress@arcor.de>

   pinentry-efl is a pinentry application for the EFL widget set.
   It tries to follow the Gnome Human Interface Guide as close as
   possible.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <Elementary.h>
#include <Ecore_X.h>
#include <gpg-error.h>
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#endif
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
#pragma GCC diagnostic pop
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt.h"
#endif /* HAVE_GETOPT_H */

#include "pinentry.h"

#ifdef FALLBACK_CURSES
#include "pinentry-curses.h"
#endif

#define PGMNAME "pinentry-efl"

#ifndef VERSION
#define VERSION
#endif

#define ENTRY_HIDE "Hide entry"
#define ENTRY_SHOW "Show entry"

typedef enum { CONFIRM_CANCEL, CONFIRM_OK, CONFIRM_NOTOK } confirm_value_t;

static const int WIDTH = 480;
static const int BUTTON_HEIGHT = 27;
static const int BUTTON_WIDTH = 70;
static const int BUTTON_ICON_SIZE = 13;
static const int PADDING = 5;

static Eina_Bool got_input;
static Ecore_Timer *timer;
static Evas_Object *check_label;
static Evas_Object *error_label;
static Evas_Object *entry;
static Evas_Object *repeat_entry;
static Evas_Object *qualitybar;
static Evas_Object *win;
static char **pargv;
static int grab_failed;
static int passphrase_ok;
static int confirm_mode;
static int pargc;
static confirm_value_t confirm_value;
static pinentry_t pinentry;

pinentry_cmd_handler_t pinentry_cmd_handler;

static void
quit (void)
{
  evas_object_del(win);
  elm_exit();
  ecore_main_loop_quit ();
}

static void
delete_event (void *data EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              void *event EINA_UNUSED)
{
  pinentry->close_button = 1;
  quit ();
}

static void
changed_text_handler (void *data EINA_UNUSED,
                      Evas_Object *obj,
                      void *event EINA_UNUSED)
{
  const char *s;
  int length;
  int percent;

  got_input = EINA_TRUE;

  if (pinentry->repeat_passphrase && repeat_entry)
    {
      elm_object_text_set (repeat_entry, "");
      elm_object_text_set (error_label, "");
    }

  if (!qualitybar || !pinentry->quality_bar)
    return;

  s = elm_object_text_get (obj);
  if (!s)
    s = "";
  length = strlen (s);
  percent = length? pinentry_inq_quality (pinentry, s, length) : 0;
  evas_object_color_set(qualitybar,
                        255 - ( 2.55 * percent ),
                        2.55 * percent, 0, 255);
  elm_progressbar_value_set (qualitybar, (double) percent / 100.0);
}

static void
on_check (void *data EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
  if(elm_check_state_get(obj))
    {
        elm_entry_password_set(entry, EINA_FALSE);
        elm_object_text_set(check_label,ENTRY_HIDE);
    }
  else
    {
        elm_entry_password_set(entry, EINA_TRUE);
        elm_object_text_set(check_label,ENTRY_SHOW);
    }
  evas_object_size_hint_min_set(check_label,
                                ELM_SCALE_SIZE(BUTTON_WIDTH),
                                ELM_SCALE_SIZE(BUTTON_HEIGHT));
  evas_object_size_hint_align_set(check_label, 0, 1);
}

static void
on_click (void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
  if (confirm_mode)
    {
      confirm_value = (confirm_value_t) data;
      quit ();
      return;
    }

  if (data)
    {
      const char *s;
      const char *s2;

      s = elm_entry_entry_get (entry);
      if (!s)
	s = "";

      if (pinentry->repeat_passphrase && repeat_entry)
	{
	  s2 = elm_entry_entry_get (repeat_entry);
	  if (!s2)
	    s2 = "";
	  if (strcmp (s, s2))
	    {
              elm_object_text_set(error_label,
                                  pinentry->repeat_error_string?
                                  pinentry->repeat_error_string:
				   "not correctly repeated");
              elm_object_focus_set(entry,EINA_TRUE);
              return;
	    }
	  pinentry->repeat_okay = 1;
	}

      passphrase_ok = 1;
      pinentry_setbufferlen (pinentry, strlen (s) + 1);
      if (pinentry->pin)
	strncpy (pinentry->pin, s, strlen(s) + 1);
    }
  quit ();
}

static void
enter_callback (void *data, Evas_Object * obj, void *event_info EINA_UNUSED)
{
  if (data)
    elm_object_focus_set (data, 1);
  else
    on_click ((void *) CONFIRM_OK, obj, NULL);
}

static Eina_Bool
timeout_cb (const void * data)
{
  pinentry_t pe = (pinentry_t)data;
  if (!got_input)
    {
      ecore_main_loop_quit();
      if (pe)
        pe->specific_err = gpg_error (GPG_ERR_TIMEOUT);
    }

  timer = NULL;
  return ECORE_CALLBACK_DONE;
}

static void
create_window (void)
{
  char *txt;
  Evas_Object *icon;
  Evas_Object *obj;
  Evas_Object *table;
  int btn_txt_len = 0;
  int row = 0;
  int ok_len = 0;

  win = elm_win_util_dialog_add(NULL,"pinentry","enter pin");
  elm_win_autodel_set(win, EINA_TRUE);
  elm_win_center(win,EINA_TRUE,EINA_TRUE);
  evas_object_smart_callback_add(win, "delete,request", delete_event, NULL);

  table = elm_table_add(win);
  elm_table_padding_set(table,ELM_SCALE_SIZE(PADDING),0);
  evas_object_size_hint_padding_set (table,
                                     ELM_SCALE_SIZE(PADDING),
                                     ELM_SCALE_SIZE(PADDING),
                                     ELM_SCALE_SIZE(PADDING),
                                     ELM_SCALE_SIZE(PADDING));
  evas_object_show(table);

  if (pinentry->title)
    {
      txt = pinentry_utf8_to_local (pinentry->lc_ctype,
                                    pinentry->title);
      elm_win_title_set ( win, txt );
      free (txt);
    }

  /* Description Label */
  if (pinentry->description)
    {
      char* aligned;
      int len;

      obj = elm_label_add(table);
      elm_label_line_wrap_set (obj, ELM_WRAP_WORD);
      txt = pinentry_utf8_to_local (pinentry->lc_ctype, pinentry->description);
      len = strlen(txt)+20; // 20 chars for align tag
      aligned = calloc(len+1,sizeof(char));
      if(aligned)
        {
          snprintf(aligned,len, "<align=left>%s</align>",txt);
          elm_object_text_set(obj,aligned);
          free (aligned);
        } else
          elm_object_text_set(obj,txt);
      free (txt);
      evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, 0);
      evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, 0);
      elm_table_pack(table, obj, 1, row, 5, 1);
      evas_object_show(obj);
      row++;
    }
  if (!confirm_mode && (pinentry->error || pinentry->repeat_passphrase))
    {
    /* Error Label */
    if (pinentry->error)
        txt = pinentry_utf8_to_local (pinentry->lc_ctype, pinentry->error);
      else
        txt = "";
      obj = elm_label_add(table);
      evas_object_color_set(obj, 255, 0, 0, 255);
      elm_object_text_set(obj,txt);
      elm_object_style_set(obj,"slide_bounce");
      elm_label_slide_duration_set(obj, 10);
      elm_label_slide_mode_set(obj, ELM_LABEL_SLIDE_MODE_ALWAYS);
      elm_label_slide_go(obj);
      evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, 0);
      evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, 0);
      elm_table_pack(table, obj, 1, row, 5, 1);
      evas_object_show(obj);
      if (pinentry->error)
        free (txt);
      row++;
    }

  qualitybar = NULL;

  if (!confirm_mode)
    {

    if (pinentry->prompt)
      {
        /* Entry/Prompt Label */
        obj = elm_label_add(table);
        txt = pinentry_utf8_to_local (pinentry->lc_ctype, pinentry->prompt);
        elm_object_text_set(obj,txt);
        free (txt);
        evas_object_size_hint_weight_set(obj, 0, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(obj, 1, EVAS_HINT_FILL);
        elm_table_pack(table, obj, 1, row, 1, 1);
        evas_object_show(obj);
      }

      entry = elm_entry_add(table);
      elm_entry_scrollable_set(entry, EINA_TRUE);
      elm_scroller_policy_set(entry,
                              ELM_SCROLLER_POLICY_OFF,
                              ELM_SCROLLER_POLICY_OFF);
      elm_entry_password_set(entry, EINA_TRUE);
      elm_entry_single_line_set(entry, EINA_TRUE);
      evas_object_size_hint_weight_set(entry, 0, 0);
      evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, 0);
      elm_table_pack(table, entry, 2, row, 4, 1);
      evas_object_smart_callback_add(entry,
                                     "changed",
                                     changed_text_handler,
                                     NULL);
      evas_object_show(entry);
      row++;

      /* Check box */
      obj = elm_check_add(table);
      evas_object_size_hint_align_set(obj, 1, EVAS_HINT_FILL);
      elm_table_pack(table, obj, 1, row, 1, 1);
      evas_object_smart_callback_add(obj, "changed", on_check, NULL);
      evas_object_show(obj);

      /* Check Label */
      check_label = elm_label_add(table);
      on_check((void *)NULL, obj, (void *)NULL);
      elm_table_pack(table, check_label, 2, row, 4, 1);
      evas_object_show(check_label);
      row++;

      if (pinentry->quality_bar)
	{
          /* Quality Bar Label */
	  obj = elm_label_add(table);
          txt = pinentry_utf8_to_local (pinentry->lc_ctype,
                                        pinentry->quality_bar);
          elm_object_text_set(obj,txt);
          free (txt);
          evas_object_size_hint_weight_set(obj, 0, EVAS_HINT_EXPAND);
          evas_object_size_hint_align_set(obj, 1, EVAS_HINT_FILL);
          elm_table_pack(table, obj, 1, row, 1, 1);
          evas_object_show(obj);

	  qualitybar = elm_progressbar_add(table);
          evas_object_color_set(qualitybar, 255, 0, 0, 255);
          evas_object_show(qualitybar);
          if (pinentry->quality_bar_tt)
	    elm_object_tooltip_text_set (qualitybar,
					 pinentry->quality_bar_tt);
          evas_object_size_hint_weight_set(qualitybar, EVAS_HINT_EXPAND, 0);
          evas_object_size_hint_align_set(qualitybar, EVAS_HINT_FILL, 0);
          elm_table_pack(table, qualitybar, 2, row, 4, 1);
          row++;
	}

      if (pinentry->repeat_passphrase)
        {
          /* Repeat Label */
	  obj = elm_label_add(table);
          txt = pinentry_utf8_to_local (pinentry->lc_ctype,
                                        pinentry->repeat_passphrase);
          elm_object_text_set(obj,txt);
          free (txt);
          evas_object_size_hint_weight_set(obj, 0, EVAS_HINT_EXPAND);
          evas_object_size_hint_align_set(obj, 1, EVAS_HINT_FILL);
          elm_table_pack(table, obj, 1, row, 1, 1);
          evas_object_show(obj);

          repeat_entry = elm_entry_add(table);
          elm_entry_scrollable_set(repeat_entry, EINA_TRUE);
          elm_scroller_policy_set(repeat_entry,
                                  ELM_SCROLLER_POLICY_OFF,
                                  ELM_SCROLLER_POLICY_OFF);
          elm_entry_password_set(repeat_entry, EINA_TRUE);
          elm_entry_single_line_set(repeat_entry, EINA_TRUE);
          evas_object_size_hint_weight_set(repeat_entry, 0, 0);
          evas_object_size_hint_align_set(repeat_entry, EVAS_HINT_FILL, 0);
          elm_table_pack(table, repeat_entry, 2, row, 4, 1);
	  evas_object_smart_callback_add (repeat_entry, "activated",
					  enter_callback, NULL);
          evas_object_show(repeat_entry);
          evas_object_smart_callback_add (entry,
                                          "activated",
                                          enter_callback,
                                          repeat_entry);
          evas_object_smart_callback_add(repeat_entry,
                                         "activated",
                                         on_click,
                                         (void *) CONFIRM_OK);
          row++;
        }
      else
        evas_object_smart_callback_add(entry,
                                       "activated",
                                       on_click,
                                       (void *) CONFIRM_OK);
  }

  /* Cancel Button */
  if (!pinentry->one_button)
    {
      obj = elm_button_add(table);
      icon = elm_icon_add (table);
      evas_object_size_hint_aspect_set (icon, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
      if (elm_icon_standard_set (icon, "dialog-cancel") ||
          elm_icon_standard_set (icon, "window-close"))
        {
          evas_object_size_hint_min_set(icon,
                                        ELM_SCALE_SIZE(BUTTON_ICON_SIZE),
                                        ELM_SCALE_SIZE(BUTTON_ICON_SIZE));
          elm_object_part_content_set(obj, "icon", icon);
          evas_object_show (icon);
        }
      else
        evas_object_del(icon);
      if (pinentry->cancel || pinentry->default_cancel)
        {
          if(pinentry->cancel)
            txt = pinentry_utf8_to_local (pinentry->lc_ctype, pinentry->cancel);
          else
            txt = pinentry_utf8_to_local (pinentry->lc_ctype,
                                          pinentry->default_cancel);
          if(txt[0]=='_')
            elm_object_text_set(obj,txt+1);
          else
            elm_object_text_set(obj,txt);
          btn_txt_len = ELM_SCALE_SIZE(strlen(txt) * (PADDING * 1.5));
          free (txt);
        }
      else
        elm_object_text_set(obj, "Cancel"); //STOCK_CANCEL
      evas_object_size_hint_align_set(obj, 0, 0);
      if(btn_txt_len>ELM_SCALE_SIZE(BUTTON_WIDTH))
        evas_object_size_hint_min_set(obj,
                                      btn_txt_len,
                                      ELM_SCALE_SIZE(BUTTON_HEIGHT));
      else
        evas_object_size_hint_min_set(obj,
                                      ELM_SCALE_SIZE(BUTTON_WIDTH),
                                      ELM_SCALE_SIZE(BUTTON_HEIGHT));
      elm_table_pack(table, obj, 4, row, 1, 1);
      evas_object_smart_callback_add(obj,
                                     "clicked",
                                     on_click,
                                     (void *) CONFIRM_CANCEL);
      evas_object_show(obj);
    }

  /* OK Button */
  obj = elm_button_add(table);
  icon = elm_icon_add (table);
  evas_object_size_hint_aspect_set (icon, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
  if (elm_icon_standard_set (icon, "dialog-ok") ||
      elm_icon_standard_set (icon, "list-add"))
    {
      evas_object_size_hint_min_set(icon,
                                    ELM_SCALE_SIZE(BUTTON_ICON_SIZE),
                                    ELM_SCALE_SIZE(BUTTON_ICON_SIZE));
      elm_object_part_content_set(obj, "icon", icon);
      evas_object_show (icon);
    }
  else
    evas_object_del(icon);
  if (pinentry->ok || pinentry->default_ok)
    {
      if(pinentry->ok)
        txt = pinentry_utf8_to_local (pinentry->lc_ctype, pinentry->ok);
      else
        txt = pinentry_utf8_to_local (pinentry->lc_ctype, pinentry->default_ok);
      if(txt[0]=='_')
        elm_object_text_set(obj,txt+1);
      else
        elm_object_text_set(obj,txt);
      ok_len = ELM_SCALE_SIZE(strlen(txt) * (PADDING * 1.5));
      if(ok_len>btn_txt_len)
        btn_txt_len = ok_len;
      free (txt);
    }
  else
    elm_object_text_set(obj,"OK"); //STOCK_OK
  evas_object_size_hint_align_set(obj, 0, 0);
  if(btn_txt_len>ELM_SCALE_SIZE(BUTTON_WIDTH))
    evas_object_size_hint_min_set(obj,
                                  btn_txt_len,
                                  ELM_SCALE_SIZE(BUTTON_HEIGHT));
  else
    evas_object_size_hint_min_set(obj,
                                  ELM_SCALE_SIZE(BUTTON_WIDTH),
                                  ELM_SCALE_SIZE(BUTTON_HEIGHT));
  elm_table_pack(table, obj, 5, row, 1, 1);
  evas_object_smart_callback_add(obj, "clicked", on_click, (void *) CONFIRM_OK);
  evas_object_show(obj);

  /* Key/Lock Icon */
  obj = elm_icon_add (win);
  evas_object_size_hint_aspect_set (obj, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
  if (elm_icon_standard_set (obj, "dialog-password"))
    {
      double ic_size = WIDTH/5;
      if(row==0)
        ic_size = ic_size/3.5;
      else if(row<4)
        ic_size = ic_size - ic_size/row;
      evas_object_size_hint_min_set(obj,
                                    ELM_SCALE_SIZE(ic_size),
                                    ELM_SCALE_SIZE(ic_size));
      evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
      evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, 0.5);
      elm_table_pack(table, obj, 0, 0, 1, row? row:1);
      evas_object_show (obj);
    }
  else
      evas_object_del(obj);

  /* Box for padding */
  obj = elm_box_add (win);
  elm_box_pack_end (obj, table);
  evas_object_show (obj);

  elm_win_resize_object_add(win,obj);
  evas_object_show(win);

  if(entry)
    elm_object_focus_set (entry, EINA_TRUE);

  if (pinentry->timeout > 0)
    timer = ecore_timer_add (pinentry->timeout,
                             (Ecore_Task_Cb)timeout_cb,
                             pinentry);
}

static int
efl_cmd_handler (pinentry_t pe)
{
  int want_pass = !!pe->pin;

  got_input = EINA_FALSE;
  pinentry = pe;
  confirm_value = CONFIRM_CANCEL;
  passphrase_ok = 0;
  confirm_mode = want_pass ? 0 : 1;
  /* init ecore-x explicitly using DISPLAY since this can launch
   * from console
   */
  if (pe->display)
    ecore_x_init (pe->display);
  elm_init (pargc, pargv);
  create_window ();
  ecore_main_loop_begin ();

  if (timer)
    {
      ecore_timer_del (timer);
      timer = NULL;
    }

  if (confirm_value == CONFIRM_CANCEL || grab_failed)
    pe->canceled = 1;

  pinentry = NULL;
  if (want_pass)
    {
      if (passphrase_ok && pe->pin)
	return strlen (pe->pin);
      else
	return -1;
    }
  else
    return (confirm_value == CONFIRM_OK) ? 1 : 0;
}

int
main (int argc, char *argv[])
{
  pinentry_init (PGMNAME);

#ifdef FALLBACK_CURSES
  if (pinentry_have_display (argc, argv))
    {
#endif

  pinentry_cmd_handler = efl_cmd_handler;
  pargc = argc;
  pargv = argv;

#ifdef FALLBACK_CURSES
    }
  else
    {
      pinentry_cmd_handler = curses_cmd_handler;
    }
#endif

  pinentry_parse_opts (argc, argv);
  if (pinentry_loop ())
    return 1;

  return 0;
}
