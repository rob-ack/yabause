#!/usr/bin/env bash

# This file is part of The RetroPie Project
#
# The RetroPie Project is the legal property of its developers, whose names are
# too numerous to list here. Please refer to the COPYRIGHT.md file distributed with this source.
#
# See the LICENSE.md file at the top-level directory of this distribution and
# at https://raw.githubusercontent.com/RetroPie/RetroPie-Setup/master/LICENSE.md
#
# this file is supposed to reside in /home/pi/RetroPie-Setup/scriptmodules/libretrocores

rp_module_id="lr-yabasanshiro"
rp_module_desc="SEGA Saturn emulator Yaba Sanshiro 2"
rp_module_help="ROM Extensions: .iso .cue .zip .ccd .mds\n\nCopy your Sega Saturn & ST-V roms to $romdir/saturn\n\nCopy the required BIOS file saturn_bios.bin / stvbios.zip to $biosdir/yabasanshiro"
rp_module_licence="GPL2 https://github.com/devmiyax/yabause/blob/master/LICENSE"
#rp_module_repo="git https://github.com/libretro/yabause.git yabasanshiro 73c67668"
#rp_module_repo="git https://github.com/devmiyax/yabause.git pi4-1-9-0"
rp_module_repo="git https://github.com/rob-ack/yabause.git libretro-cmake-rpi4"
rp_module_section="exp"
rp_module_flags="!all rpi4 !videocore"

function depends_yabasanshiro() {
    local depends=(cmake pkg-config libsecret-1-dev libssl-dev libsdl2-dev libboost-all-dev)
    getDepends "${depends[@]}"
}

function sources_lr-yabasanshiro() {
    gitPullOrClone
}

function build_lr-yabasanshiro() {
    local params=()
    cd yabause/
    rm -f ./out/CMakeCache.txt
    cmake -DYAB_PORTS:STRING="libretro" -DYAB_USE_QT5:BOOL=False -DSH2_DYNAREC:BOOL=False -DSH2_TRACE:BOOL=False -S . -B ./out
    cmake --build ./out --config Release -- -j4
    md_ret_require="$md_build/yabause/out/src/libretro/libyabause_libretro.so"
}

function install_lr-yabasanshiro() {
    md_ret_files=(
        'yabause/out/src/libretro/libyabause_libretro.so'
        'LICENSE'
        'README.md'
    )
}

function configure_lr-yabasanshiro() {
    mkRomDir "saturn"
    ensureSystemretroconfig "saturn"

    addEmulator 1 "$md_id" "saturn" "$md_inst/libyabause_libretro.so"
    addSystem "saturn"
}

