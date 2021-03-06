/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Copyright 2013 Jiri Pirko <jiri@resnulli.us>
 */

#ifndef __NM_SETTING_TEAM_H__
#define __NM_SETTING_TEAM_H__

#if !defined (__NETWORKMANAGER_H_INSIDE__) && !defined (NETWORKMANAGER_COMPILATION)
#error "Only <NetworkManager.h> can be included directly."
#endif

#include "nm-setting.h"

G_BEGIN_DECLS

#define NM_TYPE_SETTING_TEAM            (nm_setting_team_get_type ())
#define NM_SETTING_TEAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_SETTING_TEAM, NMSettingTeam))
#define NM_SETTING_TEAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_SETTING_TEAM, NMSettingTeamClass))
#define NM_IS_SETTING_TEAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_SETTING_TEAM))
#define NM_IS_SETTING_TEAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NM_TYPE_SETTING_TEAM))
#define NM_SETTING_TEAM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_SETTING_TEAM, NMSettingTeamClass))

#define NM_SETTING_TEAM_SETTING_NAME "team"

#define NM_SETTING_TEAM_CONFIG                      "config"
#define NM_SETTING_TEAM_NOTIFY_PEERS_COUNT          "notify-peers-count"
#define NM_SETTING_TEAM_NOTIFY_PEERS_INTERVAL       "notify-peers-interval"
#define NM_SETTING_TEAM_MCAST_REJOIN_COUNT          "mcast-rejoin-count"
#define NM_SETTING_TEAM_MCAST_REJOIN_INTERVAL       "mcast-rejoin-interval"
#define NM_SETTING_TEAM_RUNNER                      "runner"
#define NM_SETTING_TEAM_RUNNER_HWADDR_POLICY        "runner-hwaddr-policy"
#define NM_SETTING_TEAM_RUNNER_TX_HASH              "runner-tx-hash"
#define NM_SETTING_TEAM_RUNNER_TX_BALANCER          "runner-tx-balancer"
#define NM_SETTING_TEAM_RUNNER_TX_BALANCER_INTERVAL "runner-tx-balancer-interval"
#define NM_SETTING_TEAM_RUNNER_ACTIVE               "runner-active"
#define NM_SETTING_TEAM_RUNNER_FAST_RATE            "runner-fast-rate"
#define NM_SETTING_TEAM_RUNNER_SYS_PRIO             "runner-sys-prio"
#define NM_SETTING_TEAM_RUNNER_MIN_PORTS            "runner-min-ports"
#define NM_SETTING_TEAM_RUNNER_AGG_SELECT_POLICY    "runner-agg-select-policy"

#define NM_SETTING_TEAM_RUNNER_BROADCAST    "broadcast"
#define NM_SETTING_TEAM_RUNNER_ROUNDROBIN   "roundrobin"
#define NM_SETTING_TEAM_RUNNER_ACTIVEBACKUP "activebackup"
#define NM_SETTING_TEAM_RUNNER_LOADBALANCE  "loadbalance"
#define NM_SETTING_TEAM_RUNNER_LACP         "lacp"

#define NM_SETTING_TEAM_RUNNER_HWADDR_POLICY_SAME_ALL    "same_all"
#define NM_SETTING_TEAM_RUNNER_HWADDR_POLICY_BY_ACTIVE   "by_active"
#define NM_SETTING_TEAM_RUNNER_HWADDR_POLICY_ONLY_ACTIVE "only_active"

#define NM_SETTING_TEAM_RUNNER_AGG_SELECT_POLICY_LACP_PRIO        "lacp_prio"
#define NM_SETTING_TEAM_RUNNER_AGG_SELECT_POLICY_LACP_PRIO_STABLE "lacp_prio_stable"
#define NM_SETTING_TEAM_RUNNER_AGG_SELECT_POLICY_BANDWIDTH        "bandwidth"
#define NM_SETTING_TEAM_RUNNER_AGG_SELECT_POLICY_COUNT            "count"
#define NM_SETTING_TEAM_RUNNER_AGG_SELECT_POLICY_PORT_CONFIG      "port_config"

#define NM_SETTING_TEAM_NOTIFY_PEERS_COUNT_ACTIVEBACKUP_DEFAULT 1
#define NM_SETTING_TEAM_NOTIFY_MCAST_COUNT_ACTIVEBACKUP_DEFAULT 1
#define NM_SETTING_TEAM_RUNNER_DEFAULT                      NM_SETTING_TEAM_RUNNER_ROUNDROBIN
#define NM_SETTING_TEAM_RUNNER_HWADDR_POLICY_DEFAULT        NM_SETTING_TEAM_RUNNER_HWADDR_POLICY_SAME_ALL
#define NM_SETTING_TEAM_RUNNER_TX_BALANCER_INTERVAL_DEFAULT 50
#define NM_SETTING_TEAM_RUNNER_SYS_PRIO_DEFAULT             255
#define NM_SETTING_TEAM_RUNNER_AGG_SELECT_POLICY_DEFAULT    NM_SETTING_TEAM_RUNNER_AGG_SELECT_POLICY_LACP_PRIO

/**
 * NMSettingTeam:
 *
 * Teaming Settings
 */
struct _NMSettingTeam {
	NMSetting parent;
};

typedef struct {
	NMSettingClass parent;

	/*< private >*/
	gpointer padding[4];
} NMSettingTeamClass;

GType nm_setting_team_get_type (void);

NMSetting *  nm_setting_team_new                (void);

const char * nm_setting_team_get_config (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
gint nm_setting_team_get_notify_peers_count (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
gint nm_setting_team_get_notify_peers_interval (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
gint nm_setting_team_get_mcast_rejoin_count (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
gint nm_setting_team_get_mcast_rejoin_interval (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
const char * nm_setting_team_get_runner (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
const char * nm_setting_team_get_runner_hwaddr_policy (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
const char * nm_setting_team_get_runner_tx_balancer (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
gint nm_setting_team_get_runner_tx_balancer_interval (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
gboolean nm_setting_team_get_runner_active (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
gboolean nm_setting_team_get_runner_fast_rate (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
gint nm_setting_team_get_runner_sys_prio (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
gint nm_setting_team_get_runner_min_ports (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
const char * nm_setting_team_get_runner_agg_select_policy (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
gboolean nm_setting_team_remove_runner_tx_hash_by_value (NMSettingTeam *setting, const char *txhash);
NM_AVAILABLE_IN_1_12
guint nm_setting_team_get_num_runner_tx_hash (NMSettingTeam *setting);
NM_AVAILABLE_IN_1_12
const char *nm_setting_team_get_runner_tx_hash (NMSettingTeam *setting, int idx);
NM_AVAILABLE_IN_1_12
void nm_setting_team_remove_runner_tx_hash (NMSettingTeam *setting, int idx);
NM_AVAILABLE_IN_1_12
gboolean nm_setting_team_add_runner_tx_hash (NMSettingTeam *setting, const char *txhash);
G_END_DECLS

#endif /* __NM_SETTING_TEAM_H__ */
