/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Example project configuration file for CHIP.
 *
 *          This is a place to put application or project-specific overrides
 *          to the default configuration values for general CHIP features.
 *
 */

#pragma once

#define CHIP_DEVICE_CONFIG_ENABLE_COMMISSIONER_DISCOVERY 0

#define CHIP_DEVICE_CONFIG_ENABLE_PAIRING_AUTOSTART 1

// Enable extended discovery, set timeout to 24 hours
#define CHIP_DEVICE_CONFIG_ENABLE_EXTENDED_DISCOVERY 0
#define CHIP_DEVICE_CONFIG_EXTENDED_DISCOVERY_TIMEOUT_SECS (24 * 60 * 60)

#define CHIP_DEVICE_CONFIG_ENABLE_COMMISSIONABLE_DEVICE_TYPE 1

#define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID 0xFFF1
#define CHIP_DEVICE_CONFIG_DEVICE_TYPE 259 // 0x0103 On/Off Light Switch

#define CHIP_DEVICE_CONFIG_ENABLE_COMMISSIONABLE_DEVICE_NAME 1
#define CHIP_DEVICE_CONFIG_DEVICE_NAME "Matter Virtual Device"

// Enable app platform
#define CHIP_DEVICE_CONFIG_APP_PLATFORM_ENABLED 0

// overrides CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT in CHIPProjectConfig
#define CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT 16

#define CHIP_CONFIG_ENABLE_ACL_EXTENSIONS 1

// Delta sibling mode (delta-matter/SIBLING_MODE_DESIGN.md §4/§7.1): the hub rewrites this
// device's ACL at runtime. Sibling-mode layout on the Delta fabric = commissioning phone admin,
// hub CAT admin, phone CAT Operate, panel CAT View, panel CAT Operate (non-session endpoints),
// plus exempt-panel entries (node ids, Operate on the session endpoints). The panels share one
// CAT, so the number of panels is unbounded; only the panels exempt at the same time are bounded,
// by (entries - 5) * subjects = 3 * 16 = 48. Storage is static per fabric (16 fabrics), ~45 KB.
#define CHIP_CONFIG_EXAMPLE_ACCESS_CONTROL_MAX_ENTRIES_PER_FABRIC 8
#define CHIP_CONFIG_EXAMPLE_ACCESS_CONTROL_MAX_SUBJECTS_PER_ENTRY 16
#define CHIP_CONFIG_EXAMPLE_ACCESS_CONTROL_MAX_TARGETS_PER_ENTRY 4

// include the CHIPProjectConfig from config/standalone
#include <CHIPProjectConfig.h>
