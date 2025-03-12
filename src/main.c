#include <linux/init.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jaoppb");
MODULE_DESCRIPTION("A simple input remapper, that acts on scan code");

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

struct key_remap {
    __u8 *from;
    size_t from_size;
    __u8 to;
};

// Globals

static int connected_devices = 0;
static struct key_remap key_table_parsed[MAX_REMAPS];

static int parse_remap(struct key_remap *remap, char *from, __u8 to) {
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

    remap->from = kcalloc(size, sizeof(__u8), GFP_KERNEL);
    if (remap->from == NULL) {
        pr_err("Failed to allocate memory for key_remap.from\n");
        return -1;
    }

    memcpy(remap->from, result, size);
    remap->from_size = size;
    remap->to = to;

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

// Input Handler methods

static bool filter_device(struct input_handler *handler, struct input_dev *dev) {
    return strcmp(dev->name, device_name) == 0;
}

static int connect_device(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
    pr_info("Device %s connected\n", dev->name);

    connected_devices++;
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

    if (input_register_handler(&input_handler) < 0) {
        pr_err("Failed to register input handler\n");
        return -1;
    }

    if (connected_devices == 0) {
        pr_err("No devices connected\n");
        return -1;
    }

    pr_info("Input handler registered\n");

    if (parse_remaps(key_table_parsed, key_table, key_table_count) < 0) {
        pr_err("Failed to parse key tables\n");
        return -1;
    }

    pr_info("Key tables parsed\n");

    pr_info("Key tables:\n");
    for (int i = 0; i < key_table_count; i++) {
        pr_info("From: ");
        for (int j = 0; j < key_table_parsed[i].from_size; j++) {
            pr_info("%02d ", key_table_parsed[i].from[j]);
        }
        pr_info("To: %02d\n", key_table_parsed[i].to);
    }

    return 0;
}

static void __exit remap_exit(void) {
    input_unregister_handler(&input_handler);
    pr_info("Input handler unregistered\n");
}

module_init(remap_init);
module_exit(remap_exit);