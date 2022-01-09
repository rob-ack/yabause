# YabaSanshiro for RetroArena

## How to build:

### Get Source Code



```
$ apt update 
$ apt install -y git python-pip cmake build-essential protobuf-compiler libprotobuf-dev libsecret-1-dev libssl-dev libsdl2-dev libboost-all-dev 
$ git clone https://github.com/devmiyax/yabause.git
$ cd yabause
$ git submodule update --init --recursive
$ mkdir build
$ cd build
```

### Generate Makefile for ODROID XU4

```
$ cmake ../yabause -DGIT_EXECUTABLE=/usr/bin/git -DYAB_PORTS=retro_arena -DYAB_WANT_DYNAREC_DEVMIYAX=ON -DYAB_WANT_ARM7=ON -DCMAKE_TOOLCHAIN_FILE=../yabause/src/retro_arena/xu4.cmake -DCMAKE_INSTALL_PREFIX=/opt/retroarena/emulators/yabause/
```

### Generate Makefile for ODROID N2

```
$ cmake ../yabause -DGIT_EXECUTABLE=/usr/bin/git -DYAB_PORTS=retro_arena -DYAB_WANT_DYNAREC_DEVMIYAX=ON -DCMAKE_TOOLCHAIN_FILE=../yabause/src/retro_arena/n2.cmake -DCMAKE_INSTALL_PREFIX=/opt/retroarena/emulators/yabause/
```

### Generate Makefile for RockPro64

```
$ cmake ../yabause -DGIT_EXECUTABLE=/usr/bin/git -DYAB_PORTS=retro_arena -DYAB_WANT_DYNAREC_DEVMIYAX=ON -DYAB_WANT_ARM7=ON -DCMAKE_TOOLCHAIN_FILE=../yabause/src/retro_arena/rp64.cmake -DCMAKE_INSTALL_PREFIX=/opt/retroarena/emulators/yabause/
```

### Generate Makefile for Jetson Nano

```
$ cmake ../yabause -DGIT_EXECUTABLE=/usr/bin/git -DYAB_PORTS=retro_arena -DYAB_WANT_DYNAREC_DEVMIYAX=ON -DCMAKE_TOOLCHAIN_FILE=../yabause/src/retro_arena/jetson.cmake -DCMAKE_INSTALL_PREFIX=/opt/retroarena/emulators/yabause/
```


### Build

```
make
```
Ater that you are ready to run ```./src/retro_arena/yabasanshiro``` .

## Commandline Usage:

|           |                               |                                              |
|-----------|-------------------------------|----------------------------------------------|
| -b STRING | --bios STRING                 | bios file                                    |
| -i STRING | --iso STRING                  | iso/cue file                                 |
| -r NUMBER | --resolution_mode NUMBER      | 0 .. Native                                  |
|           |                               | 1 .. 4x                                      |                                      
|           |                               | 2 .. 2x                                      | 
|           |                               | 3 .. Original                                |
|           |                               | 4 .. 720p                                    | 
|           |                               | 5 .. 1080p                                   |
| -a        | --keep_aspect_rate            | |
| -s NUMBER | --scps_sync_per_frame NUMBER  | |
| -nf        | --no_frame_skip              |  disable frame skip |
| -v        | --version                     | | 
| -h        |                               | Show help information |

## KeyConfig file:

Key config file is sotred as ~/.yabasanshiro/keymap.json .

Sample file format

```json
{
  "player1": {
    "DeviceID": 0,
    "deviceGUID": "03000000550900001072000011010000",
    "deviceName": "NVIDIA Controller v01.03",
    "pdmode": 0, 
    "up": { "id": 0,"type": "hat","value": 1 },    
    "down": { "id": 0, "type": "hat", "value": 4 },
    "left": { "id": 0, "type": "hat", "value": 8 },
    "right": { "id": 0,"type": "hat","value": 2 },
    "select": {"id": 14,"type": "button","value": 1 },
    "start": {"id": 7,"type": "button","value": 1 },
    "a": { "id": 0, "type": "button", "value": 1 },
    "b": { "id": 1, "type": "button", "value": 1 },
    "c": { "id": 5, "type": "button", "value": 1 },
    "x": {"id": 2,"type": "button", "value": 1 },
    "y": { "id": 3,"type": "button", "value": 1 },
    "z": { "id": 4,"type": "button", "value": 1  },
    "l": { "id": 5, "type": "axis", "value": 1  },
    "r": { "id": 5,"type": "axis","value": 1 },
    "analogleft": { "id": 4, "type": "axis", "value": 0 },
    "analogright": { "id": 5, "type": "axis", "value": 0 },
    "analogx": { "id": 0, "type": "axis", "value": 0  },
    "analogy": { "id": 1, "type": "axis", "value": 0  }
  }
}
```

## Special functions for Retropie:

* Synchronize input settings with ~/.emulationstation/es_temporaryinput.cfg, if you don't have ~/.yabasanshiro/keymap.json .

| es_temporaryinput.cfg | YabaSanshiro                  |
|-----------------------|-------------------------------|
| up                    | up |
| down                  | down |
| left                  | left |
| right                 | right |
| a                     | a |
| b                     | b |
| rightshoulder         | c |
| a                     | x |
| b                     | y |
| leftshoulder          | z |
| lefttrigger           | l |
| righttrigger          | r |
| start                 | start |

'select' is force to mapped as 'Show/Hide Menu' button


## Retropie on Raspberry PI 4

### Build and Install

```
$ cmake ../yabause -DGIT_EXECUTABLE=/usr/bin/git -DYAB_PORTS=retro_arena -DYAB_WANT_DYNAREC_DEVMIYAX=ON -DYAB_WANT_ARM7=ON -DCMAKE_TOOLCHAIN_FILE=../yabause/src/retro_arena/pi4.cmake -DCMAKE_INSTALL_PREFIX=/opt/retropie/emulators/yabause/
$ make 
$ sudo make install
```

### Setup 

* Copy cue or chd format files to /home/pi/RetroPie/roms/saturn/

* Add these lines to /etc/emulationstation/es_systems.cfg and reboot

```xml
  <system>
    <name>saturn</name>
    <fullname>Sega Saturn</fullname>
    <path>/home/pi/RetroPie/roms/saturn</path>
    <extension>.cue .CUE .chd .CHD </extension>
    <command>/opt/retropie/emulators/yabause/yabasanshiro -i "%ROM_RAW%"</command>
    <platform>saturn</platform>
    <theme>saturn</theme>
  </system> 
```

### RetroPie Setup 

You can also use RetroPie setup command.

```markdown
cd ~/RetroPie-Setup/scriptmodules/
wget https://raw.githubusercontent.com/devmiyax/yabause/b96dbd2393a3f9482d37344fd5d1c60fa68cac35/yabause/src/retro_arena/yabasanshiro.sh
chmod 755 yabasanshiro.sh
cd ~/RetroPie-Setup/
sudo ./retropie_setup.sh
```

