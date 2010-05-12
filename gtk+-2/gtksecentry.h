/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2004 Albrecht Dreﬂ
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

/*
 * Heavily stripped down for use in pinentry-gtk-2 by Albrecht Dreﬂ
 * <albrecht.dress@arcor.de> Feb. 2004:
 *
 * The entry is now always invisible, uses secure memory methods to
 * allocate the text memory, and all potentially dangerous methods
 * (copy & paste, popup, etc.) have been removed.
 */

#ifndef __GTK_SECURE_ENTRY_H__
#define __GTK_SECURE_ENTRY_H__


#include <gtk/gtk.h>
#include "gseal-gtk-compat.h"

#ifdef __cplusplus
extern "C" {
#ifdef MAKE_EMACS_HAPPY
}
#endif				/* MAKE_EMACS_HAPPY */
#endif				/* __cplusplus */
#define GTK_TYPE_SECURE_ENTRY                  (gtk_secure_entry_get_type ())
#define GTK_SECURE_ENTRY(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_SECURE_ENTRY, GtkSecureEntry))
#define GTK_SECURE_ENTRY_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_SECURE_ENTRY, GtkSecureEntryClass))
#define GTK_IS_SECURE_ENTRY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_SECURE_ENTRY))
#define GTK_IS_SECURE_ENTRY_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_SECURE_ENTRY))
#define GTK_SECURE_ENTRY_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_SECURE_ENTRY, GtkSecureEntryClass))
typedef struct _GtkSecureEntry GtkSecureEntry;
typedef struct _GtkSecureEntryClass GtkSecureEntryClass;

struct _GtkSecureEntry {
    GtkWidget widget;

    gchar *text;

    guint overwrite_mode:1;

    guint16 text_length;	/* length in use, in chars */
    guint16 text_max_length;

    /*< private > */
    GdkWindow *text_area;
    GtkIMContext *im_context;

    gint current_pos;
    gint selection_bound;

    PangoLayout *cached_layout;
    guint cache_includes_preedit:1;

    guint need_im_reset:1;

    guint has_frame:1;

    guint activates_default:1;

    guint cursor_visible:1;

    guint in_click:1;		/* Flag so we don't select all when clicking in entry to focus in */

    guint is_cell_renderer:1;
    guint editing_canceled:1;	/* Only used by GtkCellRendererText */

    guint mouse_cursor_obscured:1;

    guint resolved_dir : 4; /* PangoDirection */

    guint button;
    guint blink_timeout;
    guint recompute_idle;
    gint scroll_offset;
    gint ascent;		/* font ascent, in pango units  */
    gint descent;		/* font descent, in pango units  */

    guint16 text_size;		/* allocated size, in bytes */
    guint16 n_bytes;		/* length in use, in bytes */

    guint16 preedit_length;	/* length of preedit string, in bytes */
    guint16 preedit_cursor;	/* offset of cursor within preedit string, in chars */

    gunichar invisible_char;

    gint width_chars;
};

struct _GtkSecureEntryClass {
    GtkWidgetClass parent_class;

    /* Action signals
     */
    void (*activate) (GtkSecureEntry * entry);
    void (*move_cursor) (GtkSecureEntry * entry,
			 GtkMovementStep step,
			 gint count, gboolean extend_selection);
    void (*insert_at_cursor) (GtkSecureEntry * entry, const gchar * str);
    void (*delete_from_cursor) (GtkSecureEntry * entry,
				GtkDeleteType type, gint count);

    /* Padding for future expansion */
    void (*_gtk_reserved1) (void);
    void (*_gtk_reserved2) (void);
    void (*_gtk_reserved3) (void);
    void (*_gtk_reserved4) (void);
};

GType
gtk_secure_entry_get_type(void)
    G_GNUC_CONST;
GtkWidget *
gtk_secure_entry_new(void);
void
gtk_secure_entry_set_invisible_char(GtkSecureEntry * entry, gunichar ch);
gunichar
gtk_secure_entry_get_invisible_char(GtkSecureEntry * entry);
void
gtk_secure_entry_set_has_frame(GtkSecureEntry * entry, gboolean setting);
gboolean
gtk_secure_entry_get_has_frame(GtkSecureEntry * entry);
/* text is truncated if needed */
void
gtk_secure_entry_set_max_length(GtkSecureEntry * entry, gint max);
gint
gtk_secure_entry_get_max_length(GtkSecureEntry * entry);
void
gtk_secure_entry_set_activates_default(GtkSecureEntry * entry,
				       gboolean setting);
gboolean
gtk_secure_entry_get_activates_default(GtkSecureEntry * entry);

void
gtk_secure_entry_set_width_chars(GtkSecureEntry * entry, gint n_chars);
gint
gtk_secure_entry_get_width_chars(GtkSecureEntry * entry);

/* Somewhat more convenient than the GtkEditable generic functions
 */
void
gtk_secure_entry_set_text(GtkSecureEntry * entry, const gchar * text);
/* returns a reference to the text */
G_CONST_RETURN gchar *
gtk_secure_entry_get_text(GtkSecureEntry * entry);

PangoLayout *
gtk_secure_entry_get_layout(GtkSecureEntry * entry);
void
gtk_secure_entry_get_layout_offsets(GtkSecureEntry * entry,
				    gint * x, gint * y);

#if GLIB_CHECK_VERSION (2,15,5)
#define GMALLOC_SIZE gsize
#else
#define GMALLOC_SIZE gulong
#endif

gpointer secentry_malloc (GMALLOC_SIZE size);
gpointer secentry_realloc (gpointer mem, GMALLOC_SIZE size);
void secentry_free (gpointer mem);

#ifdef __cplusplus
}
#endif				/* __cplusplus */


#endif				/* __GTK_SECURE_ENTRY_H__ */
