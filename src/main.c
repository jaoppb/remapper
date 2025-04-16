#include <linux/init.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/printk.h>

#include "utils.h"

#define MAX_NAME_LENGTH 1024
#define MAX_REMAPS 100

// Device name param
static char device_name[MAX_NAME_LENGTH];

module_param_string(device_name, device_name, MAX_NAME_LENGTH, 0);
MODULE_PARM_DESC(device_name, "Name of the device to be remapped");

// Key table param
static int key_table_count = 0;
static char *key_table[MAX_REMAPS];

module_param_array(key_table, charp, &key_table_count, 0);
MODULE_PARM_DESC(key_table, "Table of key mappings");

// Key remap

struct key_remap_from {
    __u32 key_code;
    __u8 *scan_codes;
    size_t scan_codes_size;
};

struct key_remap_to {
    __u32 key_code;
};

struct key_remap {
    bool mapped;
    __u32 old;
    struct key_remap_from from;
    struct key_remap_to to;
};

// Globals

static struct key_remap key_table_parsed[MAX_REMAPS] = {
    0,
};
static struct input_dev *device = NULL;

// Remap methods

static int parse_remap(struct key_remap *remap, char *from, __u32 to) {
    char *each = NULL;
    __u8 *result = kcalloc(32, sizeof(__u8), GFP_KERNEL);

    if (result == NULL) {
        pr_err("Failed to allocate memory for key\n");
        return -1;
    }

    int size = 0;
    while ((each = strsep(&from, " ")) != NULL) {
        if (size >= 32) {
            pr_err("Scan Code too long\n");
            return -1;
        }

        if (kstrtou8(each, 16, &result[size]) < 0) {
            pr_err("Failed to parse scan code\n");
            return -1;
        }

        size++;
    }

    remap->from.scan_codes = kcalloc(size, sizeof(__u8), GFP_KERNEL);
    if (remap->from.scan_codes == NULL) {
        pr_err("Failed to allocate memory for key_remap.from\n");
        return -1;
    }

    memcpy(remap->from.scan_codes, result, size);
    remap->from.scan_codes_size = size;
    remap->to.key_code = to;

    kfree(result);

    return 0;
}

static int parse_remaps(struct key_remap *remaps, char **table, int count) {
    for (int i = 0; i < count; i++) {
        __u8 to = 0;
        char *from = strsep(&table[i], ";");
        if (kstrtou8(table[i], 10, &to) < 0) {
            pr_err("Failed to parse to key\n");
            return -1;
        }

        if (parse_remap(&remaps[i], from, to) < 0) {
            return -1;
        }
    }

    return 0;
}

static int free_remaps(struct key_remap *remaps, int count) {
	for (int i = 0; i < count; i++) {
		kfree(remaps[i].from.scan_codes);
		remaps[i].from.scan_codes = NULL;
		remaps[i].from.scan_codes_size = 0;
		remaps[i].mapped = false;
		remaps[i].old = 0;
		remaps[i].from.key_code = 0;
		remaps[i].to.key_code = 0;
	}

	return 0;
}

// Remap Device methods

static int remap_device_key(struct input_dev *dev, struct key_remap *remap) {
    struct input_keymap_entry entry = {
        .keycode = remap->to.key_code,
        .len = remap->from.scan_codes_size,
    };
    memcpy(entry.scancode, remap->from.scan_codes, remap->from.scan_codes_size);

    if (dev->setkeycode(dev, &entry, &remap->old) < 0) {
        pr_err("Failed to remap key\n");
        return -1;
    }

    pr_info("Key %d remapped to %d\n", remap->old, remap->to.key_code);

    remap->mapped = true;
    return 0;
}

static int undo_remap_device_key(struct input_dev* dev, struct key_remap* remap, int remap_count) {
	for (int i = 0; i < remap_count; i++) {
		if (!remap[i].mapped) continue;

		__u8 aux = remap[i].to.key_code;
		remap[i].to.key_code = remap[i].old;
		remap_device_key(dev, &remap[i]);
		remap[i].to.key_code = aux;
		remap[i].mapped = false;
	}

    return 0;
}

// Input Handler methods

static bool filter_device(struct input_handler *handler, struct input_dev *dev) {
    return strcmp(dev->name, device_name) == 0;
}

static int connect_device(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
    if (device != NULL) {
        pr_err("Only one device can be remapped at a time\n");
        return -1;
    }

    for (int i = 0; i < key_table_count; i++) {
        if (remap_device_key(dev, &key_table_parsed[i]) < 0) {
            pr_err("Failed to remap device key\n");
            return -1;
        }
    }

    for (int i = 0; i < key_table_count; i++) {
        key_table_parsed[i].from.key_code = get_key_code(dev, key_table_parsed[i].from.scan_codes, key_table_parsed[i].from.scan_codes_size);
    }

    pr_info("Device %s connected\n", dev->name);

    device = dev;
    return 0;
}

static void disconnect_device(struct input_handle *handle) {
    pr_info("Device %s disconnected\n", handle->dev->name);
}

// Input Handler registration
static const struct input_device_id device_ids[] = {
    {.driver_info = 13},
    {}};

static struct input_handler input_handler = {
    .match = filter_device,
    .connect = connect_device,
    .disconnect = disconnect_device,
    .name = "input_handler",
    .id_table = device_ids,
};

static int __init remap_init(void) {
    if (key_table_count == 0) {
        pr_err("No key tables specified\n");
        return -1;
    }

    if (parse_remaps(key_table_parsed, key_table, key_table_count) < 0) {
        pr_err("Failed to parse key tables\n");
        return -1;
    }

    pr_info("Key tables parsed\n");

    if (input_register_handler(&input_handler) < 0) {
        pr_err("Failed to register input handler\n");
        return -1;
    }

    pr_info("Input handler registered\n");

    return 0;
}

static void __exit remap_exit(void) {
    if(device != NULL)  {
        for (int i = 0; i < key_table_count; i++) {
            if (!key_table_parsed[i].mapped) continue;

            __u8 aux = key_table_parsed[i].to.key_code;
            key_table_parsed[i].to.key_code = key_table_parsed[i].old;
            remap_device_key(device, &key_table_parsed[i]);
            key_table_parsed[i].to.key_code = aux;
            key_table_parsed[i].mapped = false;
        }
    }

    input_unregister_handler(&input_handler);
    pr_info("Input handler unregistered\n");
}

module_init(remap_init);
module_exit(remap_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jaoppb");
MODULE_DESCRIPTION("A simple input remapper, that acts on scan code");
MODULE_VERSION("1");