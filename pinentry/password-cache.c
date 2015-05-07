/* password-cache.c - Password cache support.
   Copyright (C) 2015 g10 Code GmbH

   This file is part of PINENTRY.

   PINENTRY is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   PINENTRY is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_LIBSECRET
# include <libsecret/secret.h>
#endif

#include "password-cache.h"
#include "memory.h"

#ifdef HAVE_LIBSECRET
static const SecretSchema *
gpg_schema (void)
{
    static const SecretSchema the_schema = {
        "org.gnupg.Passphrase", SECRET_SCHEMA_NONE,
        {
	  { "stored-by", SECRET_SCHEMA_ATTRIBUTE_STRING },
	  { "key-grip", SECRET_SCHEMA_ATTRIBUTE_STRING },
	  { "NULL", 0 },
	}
    };
    return &the_schema;
}

static char *
key_grip_to_label (const char *key_grip)
{
  char *label = NULL;
  if (asprintf(&label, "GnuPG: %s", key_grip) < 0)
    return NULL;
  return label;
}
#endif

void
password_cache_save (const char *key_grip, const char *password)
{
#ifdef HAVE_LIBSECRET
  char *label;
  GError *error = NULL;

  if (! *key_grip)
    return;

  label = key_grip_to_label (key_grip);
  if (! label)
    return;

  if (! secret_password_store_sync (gpg_schema (),
				    SECRET_COLLECTION_DEFAULT,
				    label, password, NULL, &error,
				    "stored-by", "GnuPG Pinentry",
				    "key-grip", key_grip, NULL))
    {
      printf("Failed to cache password for key %s with secret service: %s\n",
	     key_grip, error->message);

      g_error_free (error);
    }

  free (label);
#else
  return;
#endif
}

char *
password_cache_lookup (const char *key_grip)
{
#ifdef HAVE_LIBSECRET
  GError *error = NULL;
  char *password;
  char *password2;

  if (! *key_grip)
    return NULL;

  password = secret_password_lookup_nonpageable_sync
    (gpg_schema (), NULL, &error,
     "key-grip", key_grip, NULL);

  if (error != NULL)
    {
      printf("Failed to lookup password for key %s with secret service: %s\n",
	     key_grip, error->message);
      g_error_free (error);
      return NULL;
    }
  if (! password)
    /* The password for this key is not cached.  Just return NULL.  */
    return NULL;

  /* The password needs to be returned in secmem allocated memory.  */
  password2 = secmem_malloc (strlen (password) + 1);
  if (password2)
    strcpy(password2, password);
  else
    printf("secmem_malloc failed: can't copy password!\n");

  secret_password_free (password);

  return password2;
#else
  return NULL;
#endif
}
