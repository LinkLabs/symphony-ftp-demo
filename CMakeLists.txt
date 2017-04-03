cmake_minimum_required(VERSION 2.8.11)
project(symphony-ftp-demo)
set(CMAKE_BUILD_TYPE Debug)

include_directories(inc/ inc/host-interface/inc)
add_executable(symphony-ftp-demo
    src/main.c
    inc/host-interface/src/ll_ifc.c
    inc/host-interface/src/ll_ifc_ftp.c
    inc/host-interface/src/ll_ifc_no_mac.c
    inc/host-interface/src/ll_ifc_symphony.c
    inc/host-interface/src/ll_ifc_utils.c
    inc/host-interface/src/ll_ifc_time_pc.c
    inc/host-interface/src/ll_ifc_transport_pc.c
    inc/host-interface/src/ifc_struct_defs.c
    )

target_link_libraries(symphony-ftp-demo m)
