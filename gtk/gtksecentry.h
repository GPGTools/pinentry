/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GTK_SECURE_ENTRY_H__
#define __GTK_SECURE_ENTRY_H__


#include <gdk/gdk.h>
#include <gtk/gtkeditable.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_SECURE_ENTRY           (gtk_secure_entry_get_type ())
#define GTK_SECURE_ENTRY(obj)           (GTK_CHECK_CAST ((obj), GTK_TYPE_SECURE_ENTRY, GtkSecureEntry))
#define GTK_SECURE_ENTRY_CLASS(klass)   (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_SECURE_ENTRY, GtkSecureEntryClass))
#define GTK_IS_SECURE_ENTRY(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_SECURE_ENTRY))
#define GTK_IS_SECURE_ENTRY_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_SECURE_ENTRY))


typedef struct _GtkSecureEntry       GtkSecureEntry;
typedef struct _GtkSecureEntryClass  GtkSecureEntryClass;

struct _GtkSecureEntry
{
  GtkEditable editable;

  GdkWindow *text_area;
  GdkPixmap *backing_pixmap;
  GdkCursor *cursor;
  GdkWChar  *text;

  guint16 text_size;	/* allocated size */
  guint16 text_length;	/* length in use */
  guint16 text_max_length;
  gint    scroll_offset;
  guint   visible : 1;	/* deprecated - see editable->visible */
  guint32 timer;
  guint   button;

  /* The x-offset of each character (including the last insertion position)
   * only valid when the widget is realized */
  gint   *char_offset;

  /* Same as 'text', but in multibyte */
  gchar  *text_mb;
  /* If true, 'text' and 'text_mb' are not coherent */
  guint   text_mb_dirty : 1;
  /* If true, we use the encoding of wchar_t as the encoding of 'text'.
   * Otherwise we use the encoding of multi-byte characters instead. */
  guint   use_wchar : 1;
};

struct _GtkSecureEntryClass
{
  GtkEditableClass parent_class;
};

GtkType    gtk_secure_entry_get_type       		(void);
GtkWidget* gtk_secure_entry_new            		(void);
GtkWidget* gtk_secure_entry_new_with_max_length	(guint16       max);
void       gtk_secure_entry_set_text       		(GtkSecureEntry      *entry,
						 const gchar   *text);
void       gtk_secure_entry_append_text    		(GtkSecureEntry      *entry,
						 const gchar   *text);
void       gtk_secure_entry_prepend_text   		(GtkSecureEntry      *entry,
						 const gchar   *text);
void       gtk_secure_entry_set_position   		(GtkSecureEntry      *entry,
						 gint           position);
/* returns a reference to the text */
gchar*     gtk_secure_entry_get_text       		(GtkSecureEntry      *entry);
void       gtk_secure_entry_select_region  		(GtkSecureEntry      *entry,
						 gint           start,
						 gint           end);
void       gtk_secure_entry_set_visibility 		(GtkSecureEntry      *entry,
						 gboolean       visible);
void       gtk_secure_entry_set_editable   		(GtkSecureEntry      *entry,
						 gboolean       editable);
/* text is truncated if needed */
void       gtk_secure_entry_set_max_length 		(GtkSecureEntry      *entry,
						 guint16        max);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_SECURE_ENTRY_H__ */
