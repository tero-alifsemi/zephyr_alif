# Copyright 2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ARM_ETHOS_U AND CONFIG_MULTITHREADING)
    set(ETHOSU_TARGET_NPU_CONFIG ${CONFIG_ARM_ETHOS_U_NPU_NAME} CACHE STRING "")

    # Mapping log level from Zephyr (none=0, err=1, wrn=2, inf=3, dbg=4) to
    # Ethos-U driver (err=0, warn=1, info=2, debug=3)
    set(ETHOSU_LOG_SEVERITY_MAP err err warning info debug)
    list(LENGTH ETHOSU_LOG_SEVERITY_MAP ETHOSU_LOG_SEVERITY_MAP_LENGTH)

    if ("${CONFIG_ARM_ETHOS_U_LOG_LEVEL}" LESS_EQUAL "${ETHOSU_LOG_SEVERITY_MAP_LENGTH}")
        list(GET ETHOSU_LOG_SEVERITY_MAP ${CONFIG_ARM_ETHOS_U_LOG_LEVEL} ETHOSU_LOG_SEVERITY)
        set(ETHOSU_LOG_SEVERITY ${ETHOSU_LOG_SEVERITY} CACHE STRING "")
    else()
        set(ETHOSU_LOG_SEVERITY debug CACHE STRING "")
    endif()

    add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR} ethos-u-core-driver)

    if (CONFIG_SOC_FAMILY_ENSEMBLE AND NOT CONFIG_SOC_SERIES_ENSEMBLE_E1C)
        target_compile_definitions(ethosu_core_driver PRIVATE
            NPU_QCONFIG=1
            NPU_REGIONCFG_0=1
            NPU_REGIONCFG_1=0
            NPU_REGIONCFG_2=1
        )
    endif()

    # Ethos-U55 and Ethos-U65 supports up to 128 bytes,
    # and Ethos-U85 supports up to 256 bytes.
    # Although, this is system implementation dependent the platforms we build for should support the
    # maximum burst length for all NPU configurations.

    target_compile_definitions(ethosu_core_driver PUBLIC
        AXI_LIMIT0_MAX_BEATS_BYTES=1
        AXI_LIMIT1_MAX_BEATS_BYTES=1
        AXI_LIMIT2_MAX_BEATS_BYTES=1
        AXI_LIMIT3_MAX_BEATS_BYTES=1) # 0 = 64 byte burst & 1 = 128 byte burst


    # Use "normal" (modifiable) transactions, improving interconnect transit
    target_compile_definitions(ethosu_core_driver PUBLIC
        AXI_LIMIT0_MEM_TYPE=2
        AXI_LIMIT1_MEM_TYPE=2
        AXI_LIMIT2_MEM_TYPE=2
        AXI_LIMIT3_MEM_TYPE=2) # 2 = Normal Non-cacheable Non-bufferable

    target_link_libraries(ethosu_core_driver PUBLIC
       zephyr_interface)

    zephyr_link_libraries(ethosu_core_driver)
endif()
