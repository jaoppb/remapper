# Remapper

This is a linux kernel module to remmap a device scancode to a keycode.

## How to use

As any module you will need to ```modprobe``` or ```insmod``` to load it. There are two parameters that you need to pass:
 * ```device_name```: any device name.
 * ```key_table```: can receive multiple remaps and is formatted as ```[scancode1];[keycode1],[scancode2];[keycode2],...```, has a limit of 100 remaps, but can be changed at code. Example: ```0x25;0```, it remaps my KP_ASTERISK to reserved, thus effectively disabling it.

Final command example:
```bash
insmod remapper device_name="AT Translated Set 2 keyboard" key_table="0x25;0"
```

## Future Changes

A secition dedicated to some future features:

- [ ] Multiple Devices
- [ ] Helper Scrit to find scan codes and key codes

## Motivation

I made this to fix a Ghost Key that I have from a notebook, thus hard to replace the keyboard. Some alternatives to fix that problem or didn't solve the problem or didn't satisfy me enough. I wanted something that could be enabled essentially at early as possible so I could for example don't worry if my crypt password has some unknown chars in the middle.