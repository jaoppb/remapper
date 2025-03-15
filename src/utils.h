#pragma once

#include <linux/input.h>

static int get_key_code(struct input_dev *dev, __u8 *scan_codes, size_t size) {
    struct input_keymap_entry entry = {
        .len = size,
    };
    memcpy(entry.scancode, scan_codes, size);

    if (dev->getkeycode(dev, &entry) < 0) {
        pr_err("Failed to get key code\n");
        return -1;
    }

    return entry.keycode;
}