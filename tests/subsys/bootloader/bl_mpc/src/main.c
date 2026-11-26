/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/ztest.h>

#include <bl_mpc.h>

#if defined(CONFIG_TEST_BL_MPC_FAKE)

#include "mpc_fakes.h"

static const struct bl_mpc_region regions[] = {
	{
		.start = 0x1000U,
		.end = 0x2000U,
		.permissions = BL_MPC_PERM_RX,
	},
	{
		.start = 0x3000U,
		.end = 0x5000U,
		.permissions = BL_MPC_PERM_RW,
	},
};

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	fake_mpc_reset();
}

ZTEST(bl_mpc, test_invalid_input)
{
	struct bl_mpc_region invalid = regions[0];

	/* NULL pointer or region count. */
	zassert_equal(bl_mpc_protect_and_lock(NULL, 1U), -EINVAL);
	zassert_equal(bl_mpc_protect_and_lock(regions, 0U), -EINVAL);
	zassert_equal(bl_mpc_protect_and_lock(regions, BL_MPC_REGION_COUNT_MAX + 1U), -ENOSPC);

	/* Misaligned start address. */
	invalid.start++;
	zassert_equal(bl_mpc_protect_and_lock(&invalid, 1U), -EINVAL);

	/* Misaligned end address. */
	invalid = regions[0];
	invalid.end++;
	zassert_equal(bl_mpc_protect_and_lock(&invalid, 1U), -EINVAL);

	/* End address same as start address. */
	invalid = regions[0];
	invalid.end = invalid.start;
	zassert_equal(bl_mpc_protect_and_lock(&invalid, 1U), -EINVAL);

	/* End address before start address. */
	invalid = regions[0];
	invalid.end = invalid.start - BL_MPC_GRANULARITY_BYTES;
	zassert_equal(bl_mpc_protect_and_lock(&invalid, 1U), -EINVAL);

	/* Invalid permission bits. */
	invalid = regions[0];
	invalid.permissions = BL_MPC_PERM_ALL | (1U << 3);
	zassert_equal(bl_mpc_protect_and_lock(&invalid, 1U), -EINVAL);
	zassert_equal(fake_mpc_write_count_get(), 0U);
	zassert_equal(fake_mpc_disable_count_get(), 0U);
}

ZTEST(bl_mpc, test_program_and_reject_repeat)
{
	zassert_ok(bl_mpc_protect_and_lock(regions, ARRAY_SIZE(regions)));
	zassert_equal(fake_mpc_write_count_get(), ARRAY_SIZE(regions));

	for (uint8_t i = 0U; i < ARRAY_SIZE(regions); i++) {
		const struct fake_mpc_override *state = fake_mpc_override_get(i);

		zassert_true(state->enabled);
		zassert_true(state->locked);
		zassert_mem_equal(&state->region, &regions[i], sizeof(regions[i]));
	}

	for (uint8_t i = ARRAY_SIZE(regions); i <= BL_MPC_REGION_COUNT_MAX; i++) {
		const struct fake_mpc_override *state = fake_mpc_override_get(i);

		zassert_false(state->enabled);
		zassert_true(state->locked);
	}

	uint32_t write_count = fake_mpc_write_count_get();
	uint32_t disable_count = fake_mpc_disable_count_get();

	zassert_equal(bl_mpc_protect_and_lock(regions, ARRAY_SIZE(regions)), -EACCES);
	zassert_equal(fake_mpc_write_count_get(), write_count);
	zassert_equal(fake_mpc_disable_count_get(), disable_count);
}

ZTEST(bl_mpc, test_previous_configuration)
{
	struct fake_mpc_override state = {
		.region = regions[0],
		.enabled = true,
	};

	fake_mpc_override_set(0U, &state);
	zassert_equal(bl_mpc_protect_and_lock(regions, ARRAY_SIZE(regions)), -EACCES);

	fake_mpc_reset();
	state = (struct fake_mpc_override){
		.region = regions[0],
		.enabled = false,
		.locked = true,
	};

	fake_mpc_override_set(0U, &state);
	zassert_equal(bl_mpc_protect_and_lock(regions, ARRAY_SIZE(regions)), -EACCES);

	fake_mpc_reset();
	state.enabled = true;
	fake_mpc_override_set(0U, &state);
	zassert_equal(bl_mpc_protect_and_lock(regions, ARRAY_SIZE(regions)), -EACCES);

	fake_mpc_reset();
	state.enabled = true;
	state.region.permissions = BL_MPC_PERM_RW;
	fake_mpc_override_set(0U, &state);
	zassert_equal(bl_mpc_protect_and_lock(regions, ARRAY_SIZE(regions)), -EACCES);

	fake_mpc_reset();
	state.region = regions[0];
	fake_mpc_override_set(ARRAY_SIZE(regions), &state);
	zassert_equal(bl_mpc_protect_and_lock(regions, ARRAY_SIZE(regions)), -EACCES);
	zassert_equal(fake_mpc_write_count_get(), 0U);
	zassert_equal(fake_mpc_disable_count_get(), 0U);
}

ZTEST(bl_mpc, test_readback_failure)
{
	fake_mpc_set_match_failure(0U);
	zassert_equal(bl_mpc_protect_and_lock(regions, ARRAY_SIZE(regions)), -EIO);
	zassert_equal(bl_mpc_protect_and_lock(regions, ARRAY_SIZE(regions)), -EACCES);
}

ZTEST_SUITE(bl_mpc, NULL, NULL, before, NULL, NULL);

#elif defined(CONFIG_TEST_BL_MPC_LOCKED_CONFLICT)

#include <hal/nrf_mpc.h>

ZTEST(bl_mpc, test_locked_conflict)
{
	static const struct bl_mpc_region requested = {
		.start = 0x20000000U,
		.end = 0x20001000U,
		.permissions = BL_MPC_PERM_RW,
	};
	nrf_mpc_override_config_t config = {
		.lock = true,
		.enable = true,
	};

	nrf_mpc_override_startaddr_set(NRF_MPC00, 0U, requested.start);
	nrf_mpc_override_endaddr_set(NRF_MPC00, 0U, requested.end);
	nrf_mpc_override_perm_set(NRF_MPC00, 0U, NRF_MPC_PERM_READ_MASK);
	nrf_mpc_override_config_set(NRF_MPC00, 0U, &config);

	zassert_equal(bl_mpc_protect_and_lock(&requested, 1U), -EACCES);
}

ZTEST_SUITE(bl_mpc, NULL, NULL, NULL, NULL, NULL);

#else

static uint8_t protected_page[BL_MPC_GRANULARITY_BYTES] __aligned(BL_MPC_GRANULARITY_BYTES);
static uint32_t expected_faults;
static uint32_t actual_faults;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(esf);
	actual_faults++;
}

ZTEST(bl_mpc, test_protect_verify_and_fault)
{
	const struct bl_mpc_region region = {
		.start = (uint32_t)(uintptr_t)protected_page,
		.end = (uint32_t)(uintptr_t)(protected_page + sizeof(protected_page)),
		.permissions = BL_MPC_PERM_R,
	};
	volatile uint8_t *protected_byte = protected_page;

	protected_page[0] = 0U;

	zassert_ok(bl_mpc_protect_and_lock(&region, 1U));

	expected_faults++;
	*protected_byte = 1U;
	zassert_equal(actual_faults, expected_faults);
}

ZTEST_SUITE(bl_mpc, NULL, NULL, NULL, NULL, NULL);

#endif
