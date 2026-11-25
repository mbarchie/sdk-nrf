/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MPC_FAKES_H_
#define MPC_FAKES_H_

#include <stdbool.h>
#include <stdint.h>

#include <bl_mpc.h>

struct fake_mpc_override {
	struct bl_mpc_region region;
	bool enabled;
	bool locked;
};

void fake_mpc_reset(void);

void fake_mpc_override_set(uint8_t index, const struct fake_mpc_override *state);

const struct fake_mpc_override *fake_mpc_override_get(uint8_t index);

void fake_mpc_set_match_failure(uint8_t index);

uint32_t fake_mpc_write_count_get(void);

uint32_t fake_mpc_disable_count_get(void);

#endif /* MPC_FAKES_H_ */
