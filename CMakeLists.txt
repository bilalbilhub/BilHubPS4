cmake_minimum_required(VERSION 3.16)
project(BilHubPS4)

# OpenOrbis Toolchain
if(NOT DEFINED ENV{OO_PS4_TOOLCHAIN})
    message(FATAL_ERROR "OO_PS4_TOOLCHAIN environment variable not set!\nInstall from: https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain")
endif()

set(TOOLCHAIN $ENV{OO_PS4_TOOLCHAIN})

set(CMAKE_C_COMPILER   "${TOOLCHAIN}/bin/clang")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN}/bin/clang++")
set(CMAKE_AR           "${TOOLCHAIN}/bin/llvm-ar")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PS4 Compile Flags
set(PS4_FLAGS
    "--target=x86_64-scei-ps4"
    "-fPIC"
    "-funwind-tables"
    "-c"
)

set(PS4_INCLUDE_DIRS
    "${TOOLCHAIN}/include"
    "${TOOLCHAIN}/include/c++/v1"
    "src"
)

set(PS4_LIB_DIRS
    "${TOOLCHAIN}/lib"
)

# Source files
set(SOURCES
    src/main.cpp
    src/ui.cpp
    src/downloader.cpp
    src/renderer.cpp
)

# Compile each source to object
foreach(SRC ${SOURCES})
    get_filename_component(OBJ_NAME ${SRC} NAME_WE)
    set(OBJ "${CMAKE_BINARY_DIR}/${OBJ_NAME}.o")
    list(APPEND OBJECTS ${OBJ})
    add_custom_command(
        OUTPUT ${OBJ}
        COMMAND ${CMAKE_CXX_COMPILER} ${PS4_FLAGS}
            -I${TOOLCHAIN}/include
            -I${TOOLCHAIN}/include/c++/v1
            -Isrc
            ${CMAKE_SOURCE_DIR}/${SRC} -o ${OBJ}
        DEPENDS ${CMAKE_SOURCE_DIR}/${SRC}
    )
endforeach()

# Link ELF
set(ELF_OUT "${CMAKE_BINARY_DIR}/BilHubPS4.elf")
add_custom_command(
    OUTPUT ${ELF_OUT}
    COMMAND ${TOOLCHAIN}/bin/ld.lld
        -m elf_x86_64
        -pie
        --script ${TOOLCHAIN}/link.x
        --eh-frame-hdr
        -o ${ELF_OUT}
        ${OBJECTS}
        -L${TOOLCHAIN}/lib
        -lSceLibcInternal
        -lSceSysmodule_stub_weak
        -lScePad_stub_weak
        -lSceVideoOut_stub_weak
        -lSceNet_stub_weak
        -lSceHttp_stub_weak
        -lSceSsl_stub_weak
        -lSceUserService_stub_weak
        -lSceSystemService_stub_weak
        -lkernel_stub_weak
        -lc++
        -lc++abi
    DEPENDS ${OBJECTS}
)

# Generate OELF
set(OELF_OUT "${CMAKE_BINARY_DIR}/BilHubPS4.oelf")
add_custom_command(
    OUTPUT ${OELF_OUT}
    COMMAND ${TOOLCHAIN}/bin/create-fself
        -in=${ELF_OUT}
        -out=${OELF_OUT}
        --paid 0x3800000000000011
    DEPENDS ${ELF_OUT}
)

# Generate param.sfo
set(SFO_OUT "${CMAKE_BINARY_DIR}/sce_sys/param.sfo")
add_custom_command(
    OUTPUT ${SFO_OUT}
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/sce_sys
    COMMAND ${TOOLCHAIN}/bin/mksfoex
        -d ATTRIBUTE=0
        -d CATEGORY=gd
        -d APP_VER=01.00
        -d TITLE_ID=BILH00001
        -d CONTENT_ID=IV0000-BILH00001_00-BILHUBPS4000000
        "BilHubPS4"
        ${SFO_OUT}
)

# Generate PKG
add_custom_target(pkg ALL
    COMMAND ${TOOLCHAIN}/bin/pkg_gen
        --title-id BILH00001
        --passcode 00000000000000000000000000000000
        --content-id IV0000-BILH00001_00-BILHUBPS4000000
        --pkg-output-path ${CMAKE_BINARY_DIR}/BilHubPS4.pkg
        --sfo ${SFO_OUT}
        --main-module ${OELF_OUT}
        --icon0 ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png
    DEPENDS ${OELF_OUT} ${SFO_OUT}
)
