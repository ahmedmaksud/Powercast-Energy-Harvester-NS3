# CMakeLists.txt for PowerCast Energy Harvester Module
#
# Copyright (c) 2025 Texas State University
#
# Author: Ahmed Maksud <ahmed.maksud@email.ucr.edu>
# PI: Marcelo Menezes De Carvalho <mmcarvalho@txstate.edu>
# Texas State University
#
# This CMake configuration builds:
# 1. PowerCast P21XXCSR-EVB Band 6 energy harvester implementation
# 2. Deployment helper for large-scale network scenarios
# 3. Complete demo application with WiFi energy harvesting
#
# The simulation demonstrates:
# - Realistic RF-to-DC efficiency modeling for 2.4GHz WiFi
# - Advanced energy management with safety mechanisms
# - Three-tier energy model (nominal/sunk/available)
# - CSV data export for visualization and analysis

# Define source and header files
set(source_files
    ph-harvester-hardware.cc
    ph-deployment-helper.cc
)

set(header_files
    ph-harvester-hardware.h
    ph-deployment-helper.h
)

# Build PowerCast Harvester Demo - Standalone energy harvesting simulation
build_lib_example(
    NAME ph-harvester-demo
    SOURCE_FILES
        ph-harvester-demo.cc
        ${source_files}
    HEADER_FILES
        ${header_files}
    LIBRARIES_TO_LINK
        ${libcore}
        ${libnetwork}
        ${libwifi}
        ${libmobility}
        ${libinternet}
        ${libapplications}
        ${libflow-monitor}
        ${libpoint-to-point}
        ${libenergy}
)

# Copy PowerCast configuration file to build directory
add_custom_target(deploy_powercast_config ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/ph-harvester-config.txt
    ${CMAKE_BINARY_DIR}/
    
    COMMENT "Deploying PowerCast configuration files"
)

# Ensure configuration is deployed with the example
add_dependencies(ph-harvester-demo deploy_powercast_config)

# Export variables for integration
set(POWERCAST_HARDWARE_SOURCES ${source_files} PARENT_SCOPE)
set(POWERCAST_HARDWARE_HEADERS ${header_files} PARENT_SCOPE)
set(POWERCAST_HARDWARE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

message(STATUS "PowerCast Energy Harvester Module")
message(STATUS "  ✓ P21XXCSR-EVB Band 6 (2.4GHz WiFi)")
message(STATUS "  ✓ Realistic RF-to-DC efficiency modeling")
message(STATUS "  ✓ Advanced energy management with safety features")
message(STATUS "  ✓ Deployment helper for large-scale scenarios")
