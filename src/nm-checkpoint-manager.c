/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* NetworkManager -- Network link manager
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2016 Red Hat, Inc.
 */

#include "nm-default.h"

#include "nm-checkpoint-manager.h"

#include "nm-checkpoint.h"
#include "nm-connection.h"
#include "nm-core-utils.h"
#include "devices/nm-device.h"
#include "nm-exported-object.h"
#include "nm-manager.h"
#include "nm-utils.h"
#include "nm-utils/c-list.h"

NM_GOBJECT_PROPERTIES_DEFINE (NMCheckpointManager,
	PROP_CHECKPOINTS,
);

typedef struct {
	NMManager *_manager;
	GHashTable *checkpoints;
	CList list;
	guint rollback_timeout_id;
} NMCheckpointManagerPrivate;

struct _NMCheckpointManager {
	GObject parent;
	NMCheckpointManagerPrivate _priv;
};

struct _NMCheckpointManagerClass {
	GObjectClass parent;
};

G_DEFINE_TYPE (NMCheckpointManager, nm_checkpoint_manager, G_TYPE_OBJECT)

#define NM_CHECKPOINT_MANAGER_GET_PRIVATE(self) _NM_GET_PRIVATE (self, NMCheckpointManager, NM_IS_CHECKPOINT_MANAGER)

#define GET_MANAGER(priv) \
       ({ \
               typeof (priv) _priv = (priv); \
               \
               _nm_unused NMCheckpointManagerPrivate *_priv2 = _priv; \
               \
               nm_assert (_priv); \
               nm_assert (NM_IS_MANAGER (_priv->_manager)); \
               _priv->_manager; \
       })

/*****************************************************************************/

#define _NMLOG_DOMAIN      LOGD_CORE
#define _NMLOG(level, ...) __NMLOG_DEFAULT (level, _NMLOG_DOMAIN, "checkpoint", __VA_ARGS__)

/*****************************************************************************/

typedef struct {
	CList list;
	NMCheckpoint *checkpoint;
} CheckpointItem;

static void update_rollback_timeout (NMCheckpointManager *self);

static void
item_destroy (gpointer data)
{
	CheckpointItem *item = data;

	c_list_unlink (&item->list);
	nm_exported_object_unexport (NM_EXPORTED_OBJECT (item->checkpoint));
	g_object_unref (G_OBJECT (item->checkpoint));
	g_slice_free (CheckpointItem, item);
}

static gboolean
rollback_timeout_cb (NMCheckpointManager *self)
{
	NMCheckpointManagerPrivate *priv = NM_CHECKPOINT_MANAGER_GET_PRIVATE (self);
	CheckpointItem *item;
	GHashTableIter iter;
	GVariant *result;
	gint64 ts, now;
	gboolean removed = FALSE;

	now = nm_utils_get_monotonic_timestamp_ms ();

	g_hash_table_iter_init (&iter, priv->checkpoints);
	while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &item)) {
		ts = nm_checkpoint_get_rollback_ts (item->checkpoint);
		if (ts && ts <= now) {
			result = nm_checkpoint_rollback (item->checkpoint);
			if (result)
				g_variant_unref (result);
			g_hash_table_iter_remove (&iter);
			removed = TRUE;
		}
	}

	priv->rollback_timeout_id = 0;
	update_rollback_timeout (self);

	if (removed)
		_notify (self, PROP_CHECKPOINTS);

	return G_SOURCE_REMOVE;
}

static void
update_rollback_timeout (NMCheckpointManager *self)
{
	NMCheckpointManagerPrivate *priv = NM_CHECKPOINT_MANAGER_GET_PRIVATE (self);
	CheckpointItem *item;
	GHashTableIter iter;
	gint64 ts, delta, next = G_MAXINT64;

	g_hash_table_iter_init (&iter, priv->checkpoints);
	while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &item)) {
		ts = nm_checkpoint_get_rollback_ts (item->checkpoint);
		if (ts && ts < next)
			next = ts;
	}

	nm_clear_g_source (&priv->rollback_timeout_id);

	if (next != G_MAXINT64) {
		delta = MAX (next - nm_utils_get_monotonic_timestamp_ms (), 0);
		priv->rollback_timeout_id = g_timeout_add (delta,
		                                           (GSourceFunc) rollback_timeout_cb,
		                                           self);
		_LOGT ("update timeout: next check in %" G_GINT64_FORMAT " ms", delta);
	}
}

static NMCheckpoint *
find_checkpoint_for_device (NMCheckpointManager *self, NMDevice *device)
{
	NMCheckpointManagerPrivate *priv = NM_CHECKPOINT_MANAGER_GET_PRIVATE (self);
	GHashTableIter iter;
	CheckpointItem *item;

	g_hash_table_iter_init (&iter, priv->checkpoints);
	while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &item)) {
		if (nm_checkpoint_includes_device (item->checkpoint, device))
			return item->checkpoint;
	}

	return NULL;
}

NMCheckpoint *
nm_checkpoint_manager_create (NMCheckpointManager *self,
                              const char *const *device_paths,
                              guint32 rollback_timeout,
                              NMCheckpointCreateFlags flags,
                              GError **error)
{
	NMCheckpointManagerPrivate *priv;
	NMManager *manager;
	NMCheckpoint *checkpoint;
	CheckpointItem *item;
	const char * const *path;
	gs_unref_ptrarray GPtrArray *devices = NULL;
	NMDevice *device;
	const char *checkpoint_path;
	gs_free const char **device_paths_free = NULL;
	guint i;

	g_return_val_if_fail (NM_IS_CHECKPOINT_MANAGER (self), FALSE);
	g_return_val_if_fail (!error || !*error, FALSE);
	priv = NM_CHECKPOINT_MANAGER_GET_PRIVATE (self);
	manager = GET_MANAGER (priv);

	if (!device_paths || !device_paths[0]) {
		device_paths_free = nm_manager_get_device_paths (manager);
		device_paths = (const char *const *) device_paths_free;
	} else if (NM_FLAGS_HAS (flags, NM_CHECKPOINT_CREATE_FLAG_DISCONNECT_NEW_DEVICES)) {
		g_set_error_literal (error, NM_MANAGER_ERROR, NM_MANAGER_ERROR_INVALID_ARGUMENTS,
		                     "the DISCONNECT_NEW_DEVICES flag can only be used with an empty device list");
		return NULL;
	}

	devices = g_ptr_array_new ();
	for (path = device_paths; *path; path++) {
		device = nm_manager_get_device_by_path (manager, *path);
		if (!device) {
			g_set_error (error, NM_MANAGER_ERROR, NM_MANAGER_ERROR_UNKNOWN_DEVICE,
			             "device %s does not exist", *path);
			return NULL;
		}
		g_ptr_array_add (devices, device);
	}

	if (!NM_FLAGS_HAS (flags, NM_CHECKPOINT_CREATE_FLAG_DESTROY_ALL)) {
		for (i = 0; i < devices->len; i++) {
			device = devices->pdata[i];
			checkpoint = find_checkpoint_for_device (self, device);
			if (checkpoint) {
				g_set_error (error, NM_MANAGER_ERROR, NM_MANAGER_ERROR_INVALID_ARGUMENTS,
				             "device '%s' is already included in checkpoint %s",
				             nm_device_get_iface (device),
				             nm_exported_object_get_path (NM_EXPORTED_OBJECT (checkpoint)));
				return NULL;
			}
		}
	}

	checkpoint = nm_checkpoint_new (manager, devices, rollback_timeout, flags, error);
	if (!checkpoint)
		return NULL;

	if (NM_FLAGS_HAS (flags, NM_CHECKPOINT_CREATE_FLAG_DESTROY_ALL))
		g_hash_table_remove_all (priv->checkpoints);

	nm_exported_object_export (NM_EXPORTED_OBJECT (checkpoint));
	checkpoint_path = nm_exported_object_get_path (NM_EXPORTED_OBJECT (checkpoint));

	item = g_slice_new0 (CheckpointItem);
	item->checkpoint = checkpoint;
	c_list_link_tail (&priv->list, &item->list);

	if (!nm_g_hash_table_insert (priv->checkpoints,
	                             (gpointer) checkpoint_path,
	                             item))
		g_return_val_if_reached (NULL);

	_notify (self, PROP_CHECKPOINTS);
	update_rollback_timeout (self);

	return checkpoint;
}

gboolean
nm_checkpoint_manager_destroy_all (NMCheckpointManager *self,
                                   GError **error)
{
	NMCheckpointManagerPrivate *priv;

	g_return_val_if_fail (NM_IS_CHECKPOINT_MANAGER (self), FALSE);
	priv = NM_CHECKPOINT_MANAGER_GET_PRIVATE (self);

	g_hash_table_remove_all (priv->checkpoints);
	_notify (self, PROP_CHECKPOINTS);

	return TRUE;
}

gboolean
nm_checkpoint_manager_destroy (NMCheckpointManager *self,
                               const char *checkpoint_path,
                               GError **error)
{
	NMCheckpointManagerPrivate *priv;
	gboolean ret;

	g_return_val_if_fail (NM_IS_CHECKPOINT_MANAGER (self), FALSE);
	g_return_val_if_fail (checkpoint_path && checkpoint_path[0] == '/', FALSE);
	g_return_val_if_fail (!error || !*error, FALSE);

	priv = NM_CHECKPOINT_MANAGER_GET_PRIVATE (self);

	if (!nm_streq (checkpoint_path, "/")) {
		ret = g_hash_table_remove (priv->checkpoints, checkpoint_path);
		if (ret) {
			_notify (self, PROP_CHECKPOINTS);
		} else {
			g_set_error (error,
			             NM_MANAGER_ERROR,
			             NM_MANAGER_ERROR_INVALID_ARGUMENTS,
			             "checkpoint %s does not exist", checkpoint_path);
		}
		return ret;
	} else
		return nm_checkpoint_manager_destroy_all (self, error);
}

gboolean
nm_checkpoint_manager_rollback (NMCheckpointManager *self,
                                const char *checkpoint_path,
                                GVariant **results,
                                GError **error)
{
	NMCheckpointManagerPrivate *priv;
	CheckpointItem *item;

	g_return_val_if_fail (NM_IS_CHECKPOINT_MANAGER (self), FALSE);
	g_return_val_if_fail (checkpoint_path && checkpoint_path[0] == '/', FALSE);
	g_return_val_if_fail (results, FALSE);
	g_return_val_if_fail (!error || !*error, FALSE);

	priv = NM_CHECKPOINT_MANAGER_GET_PRIVATE (self);

	item = g_hash_table_lookup (priv->checkpoints, checkpoint_path);
	if (!item) {
		g_set_error (error, NM_MANAGER_ERROR, NM_MANAGER_ERROR_FAILED,
		             "checkpoint %s does not exist", checkpoint_path);
		return FALSE;
	}

	*results = nm_checkpoint_rollback (item->checkpoint);
	g_hash_table_remove (priv->checkpoints, checkpoint_path);
	_notify (self, PROP_CHECKPOINTS);

	return TRUE;
}

/*****************************************************************************/
static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
	NMCheckpointManager *self = NM_CHECKPOINT_MANAGER (object);
	NMCheckpointManagerPrivate *priv = NM_CHECKPOINT_MANAGER_GET_PRIVATE (self);
	CheckpointItem *item;
	char **strv;
	guint num, i = 0;
	CList *iter;

	switch (prop_id) {
	case PROP_CHECKPOINTS:
		num = g_hash_table_size (priv->checkpoints);
		strv = g_new (char *, num + 1);
		c_list_for_each (iter, &priv->list) {
			item = c_list_entry (iter, CheckpointItem, list);
			strv[i++] = g_strdup (nm_exported_object_get_path (NM_EXPORTED_OBJECT (item->checkpoint)));
		}
		nm_assert (i == num);
		strv[i] = NULL;
		g_value_take_boxed (value, strv);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nm_checkpoint_manager_init (NMCheckpointManager *self)
{
	NMCheckpointManagerPrivate *priv = NM_CHECKPOINT_MANAGER_GET_PRIVATE (self);

	priv->checkpoints = g_hash_table_new_full (nm_str_hash, g_str_equal,
	                                           NULL, item_destroy);
	c_list_init (&priv->list);
}

NMCheckpointManager *
nm_checkpoint_manager_new (NMManager *manager)
{
	NMCheckpointManager *self;
	NMCheckpointManagerPrivate *priv;

	self = g_object_new (NM_TYPE_CHECKPOINT_MANAGER, NULL);
	priv = NM_CHECKPOINT_MANAGER_GET_PRIVATE (self);

	/* the NMCheckpointManager instance is actually owned by NMManager.
	 * Thus, we cannot take a reference to it, and we also don't bother
	 * taking a weak-reference. Instead let GET_MANAGER() assert that
	 * self->_manager is alive -- which we always expect as the lifetime
	 * of NMManager shall surpass the lifetime of the NMCheckpointManager
	 * instance. */
	priv->_manager = manager;

	return self;
}

static void
dispose (GObject *object)
{
	NMCheckpointManager *self = NM_CHECKPOINT_MANAGER (object);
	NMCheckpointManagerPrivate *priv = NM_CHECKPOINT_MANAGER_GET_PRIVATE (self);

	nm_clear_g_source (&priv->rollback_timeout_id);
	g_clear_pointer (&priv->checkpoints, g_hash_table_unref);

	G_OBJECT_CLASS (nm_checkpoint_manager_parent_class)->dispose (object);
}

static void
nm_checkpoint_manager_class_init (NMCheckpointManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	obj_properties[PROP_CHECKPOINTS] =
	    g_param_spec_boxed (NM_CHECKPOINT_MANAGER_CHECKPOINTS, "", "",
	                        G_TYPE_STRV,
	                        G_PARAM_READABLE |
	                        G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, _PROPERTY_ENUMS_LAST, obj_properties);

	object_class->dispose = dispose;
	object_class->get_property = get_property;
}
