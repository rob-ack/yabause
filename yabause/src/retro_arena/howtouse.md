# YabaSansiro for hand held devices

YabaSansiro for hand held devices is aimed to use YabaSansiro widthout mouse and keyboard on hand held devices like Ayaneo or Next player easly. this document describe how to use.

## Requirements

* PC running Windows 10 or above
* Gapme pad( XBox controller is recomanded)
* SEGA Saturn game
* BIOS data dumped from a SEGA Saturn( not mandatry)

## Preperation

### Game files

Place game files on "Documents/YabaSanshiro/games" folder. Yabasanshiro supports "chd","cue", and "ccd" format. "chd" format is recomannded you can generate "chd" with the tool we provided. for more detail please refer [this page](https://www.uoyabause.org/static_pages/chd).

### BIOS 

The BIOS file is not recomended. but if you want to use it. rename "bios.bin" and plase it on the "Documents/YabaSanshiro" folder.

### Game pad

Connect a gamepad before running yabasanshiro.exe. A X-box style gamepad is configured automatically.

## Execution

1. Execute "yabasanshiro"
2. Push "view botton" on your game pad or "ESC" key on your keyboard. Then menu is shown up.

3. You can controll menu item with these game pad buttons
 * D-pad Down ... Move down to the next menu item 
 * D-pad Up ... Move up to the previous menu item 
 * A ... Select the menu item
 * B ... Back to the previous window

4. At the first time your game pad may not be configured. Select "Player1" and Select the game pad you are using and map buttons.

5. Select "Open CD tray"
6. Select "Close CD tray" then game list is shown up.
7. Select the game you want to play.  

## Configuration

You can config your setting by selecting 

### Resolution

* Native ... Draws at the resolution of the device you are using.
* 1080p
* 720p
* 4x
* 2x
* Original

### Aspect Rate

* Original ... Draws in the aspect ratio according to the internal settings of the Sega Saturn.
* 4:3
* 16:9
* Full screen

### Rotate Screen Resolution

Increase the resolution of RBG(Rotate BackGround Screen)

* Original
* 2x
* 720p
* 1080p
* Native

### Use Compute Shader

If it's checked, Compute Shader is used for generating hi-resolution RBG

### Rotate Screen

Rotate the screen 90 degrees for games such as Layers section