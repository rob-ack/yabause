#!/usr/bin/env bash

# This file is part of The RetroPie Project
#
# The RetroPie Project is the legal property of its developers, whose names are
# too numerous to list here. Please refer to the COPYRIGHT.md file distributed with this source.
#
# See the LICENSE.md file at the top-level directory of this distribution and
# at https://raw.githubusercontent.com/RetroPie/RetroPie-Setup/master/LICENSE.md
#

rp_module_id="lr-kronos-rob-ack"
rp_module_desc="Sega Saturn emu - Kronos Yabause (optimised) port for libretro"
rp_module_help="ROM Extensions: .iso .bin .zip\n\nCopy your Sega Saturn roms to $romdir/saturn\n\nCopy the required BIOS file saturn_bios.bin to $biosdir"
rp_module_licence="GPL2 https://raw.githubusercontent.com/libretro/yabause/master/yabause/COPYING"
rp_module_repo="git https://github.com/rob-ack/yabause.git kronos-cmake_ci"
rp_module_section="exp"
rp_module_flags="!armv6"

function sources_lr-kronos-rob-ack() {
    gitPullOrClone
}

function build_lr-kronos-rob-ack() {
    local params=()
    cd yabause/
    rm -f ./out/CMakeCache.txt
    cmake -DKRONOS_LIBRETRO_CORE:BOOL=True -DOPENGLCORE_TEST:BOOL=False -DYAB_WANT_ARM7:BOOL=True -DYAB_USE_QT5:BOOL=False -DYAB_PORTS:STRING="" -DYAB_WANT_SH2_ASYNC:BOOL=True -S . -B ./out
    cmake --build ./out --config Release -- -j4
    md_ret_require="$md_build/yabause/out/src/libretro/kronos_libretro.so"
}

function install_lr-kronos-rob-ack() {
    md_ret_files=(
        'yabause/out/src/libretro/kronos_libretro.so'
        'yabause/COPYING'
        'yabause/ChangeLog'
        'yabause/AUTHORS'
        'yabause/README'
    )
}

function configure_lr-kronos-rob-ack() {
    mkRomDir "saturn"
    ensureSystemretroconfig "saturn"

    addEmulator 1 "$md_id" "saturn" "$md_inst/kronos_libretro.so"
    addSystem "saturn"
}