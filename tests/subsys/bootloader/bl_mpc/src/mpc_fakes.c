/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/fff.h>

#include "bl_mpc_hw.h"
#include "mpc_fakes.h"

#define FAKE_OVERRIDE_COUNT (BL_MPC_REGION_COUNT_MAX + 1U)
#define NO_MATCH_FAILURE    UINT8_MAX

static struct fake_mpc_override overrides[FAKE_OVERRIDE_COUNT];
static uint8_t match_failure_index;

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC3(bl_mpc_hw_override_config_get, uint8_t, bool *, bool *);
FAKE_VALUE_FUNC2(bool, bl_mpc_hw_override_matches, uint8_t, const struct bl_mpc_region *);
FAKE_VOID_FUNC2(bl_mpc_hw_override_write, uint8_t, const struct bl_mpc_region *);
FAKE_VOID_FUNC2(bl_mpc_hw_override_disable, uint8_t, bool);
FAKE_VOID_FUNC(bl_mpc_hw_data_sync);
FAKE_VOID_FUNC(bl_mpc_hw_instruction_sync);

static void override_config_get(uint8_t index, bool *enabled, bool *locked)
{
	*enabled = overrides[index].enabled;
	*locked = overrides[index].locked;
}

static bool override_matches(uint8_t index, const struct bl_mpc_region *region)
{
	const struct bl_mpc_region *actual = &overrides[index].region;

	if (index == match_failure_index) {
		return false;
	}

	return actual->start == region->start && actual->end == region->end &&
	       actual->permissions == region->permissions;
}

static void override_write(uint8_t index, const struct bl_mpc_region *region)
{
	overrides[index].region = *region;
	overrides[index].enabled = true;
	overrides[index].locked = true;
}

static void override_disable(uint8_t index, bool lock)
{
	overrides[index].enabled = false;
	overrides[index].locked = lock;
}

void fake_mpc_reset(void)
{
	memset(overrides, 0, sizeof(overrides));
	match_failure_index = NO_MATCH_FAILURE;

	RESET_FAKE(bl_mpc_hw_override_config_get);
	RESET_FAKE(bl_mpc_hw_override_matches);
	RESET_FAKE(bl_mpc_hw_override_write);
	RESET_FAKE(bl_mpc_hw_override_disable);
	RESET_FAKE(bl_mpc_hw_data_sync);
	RESET_FAKE(bl_mpc_hw_instruction_sync);
	FFF_RESET_HISTORY();

	bl_mpc_hw_override_config_get_fake.custom_fake = override_config_get;
	bl_mpc_hw_override_matches_fake.custom_fake = override_matches;
	bl_mpc_hw_override_write_fake.custom_fake = override_write;
	bl_mpc_hw_override_disable_fake.custom_fake = override_disable;
}

void fake_mpc_override_set(uint8_t index, const struct fake_mpc_override *state)
{
	overrides[index] = *state;
}

const struct fake_mpc_override *fake_mpc_override_get(uint8_t index)
{
	return &overrides[index];
}

void fake_mpc_set_match_failure(uint8_t index)
{
	match_failure_index = index;
}

uint32_t fake_mpc_write_count_get(void)
{
	return bl_mpc_hw_override_write_fake.call_count;
}

uint32_t fake_mpc_disable_count_get(void)
{
	return bl_mpc_hw_override_disable_fake.call_count;
}
