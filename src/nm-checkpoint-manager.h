/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* NetworkManager
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
 * Copyright (C) 2016-2017 Red Hat, Inc.
 */

#ifndef __NM_CHECKPOINT_MANAGER_H__
#define __NM_CHECKPOINT_MANAGER_H__

#include "nm-dbus-interface.h"
#include "nm-checkpoint.h"

#define NM_TYPE_CHECKPOINT_MANAGER            (nm_checkpoint_manager_get_type ())
#define NM_CHECKPOINT_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_CHECKPOINT_MANAGER, NMCheckpointManager))
#define NM_CHECKPOINT_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  NM_TYPE_CHECKPOINT_MANAGER, NMCheckpointManagerClass))
#define NM_IS_CHECKPOINT_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_CHECKPOINT_MANAGER))
#define NM_IS_CHECKPOINT_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  NM_TYPE_CHECKPOINT_MANAGER))
#define NM_CHECKPOINT_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  NM_TYPE_CHECKPOINT_MANAGER, NMCheckpointManagerClass))

typedef struct _NMCheckpointManagerClass NMCheckpointManagerClass;
typedef struct _NMCheckpointManager NMCheckpointManager;

GType nm_checkpoint_manager_get_type (void);

NMCheckpointManager *nm_checkpoint_manager_new (NMManager *manager);
void nm_checkpoint_manager_unref (NMCheckpointManager *self);

NMCheckpoint *nm_checkpoint_manager_create (NMCheckpointManager *self,
                                            const char *const*device_names,
                                            guint32 rollback_timeout,
                                            NMCheckpointCreateFlags flags,
                                            GError **error);

gboolean nm_checkpoint_manager_destroy_all (NMCheckpointManager *self,
                                            GError **error);

gboolean nm_checkpoint_manager_destroy (NMCheckpointManager *self,
                                        const char *checkpoint_path,
                                        GError **error);
gboolean nm_checkpoint_manager_rollback (NMCheckpointManager *self,
                                         const char *checkpoint_path,
                                         GVariant **results,
                                         GError **error);

#endif /* __NM_CHECKPOINT_MANAGER_H__ */
