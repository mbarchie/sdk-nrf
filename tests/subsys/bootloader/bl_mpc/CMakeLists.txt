#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bl_mpc)

set(BL_MPC_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/bl_mpc)

target_include_directories(app PRIVATE ${BL_MPC_DIR})
target_sources(app PRIVATE
  ${BL_MPC_DIR}/bl_mpc.c
  src/main.c
)

if(CONFIG_TEST_BL_MPC_FAKE)
  target_compile_definitions(app PRIVATE NRF54LM20B_XXAA)
  target_sources(app PRIVATE src/mpc_fakes.c)
else()
  target_sources(app PRIVATE ${BL_MPC_DIR}/bl_mpc_hw_nrf.c)
endif()
