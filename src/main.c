#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/input.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jaoppb");
MODULE_DESCRIPTION("A simple input remapper");

#define MAX_PATH_LENGTH 1024
#define MAX_DEVICES 100

typedef __u32 keycode;

static int device_names_count = 0;
static char *device_names[MAX_PATH_LENGTH];

static int device_tables_count = 0;
static ulong device_tables[MAX_DEVICES];

module_param_array(device_names, charp, &device_names_count, 0);
MODULE_PARM_DESC(device_names, "Path to the device to be remapped");

module_param_array(device_tables, ulong, &device_tables_count, 0);
MODULE_PARM_DESC(device_tables, "Table of key mappings");

typedef struct {
    keycode from;
    keycode to;
} mapped_key;

struct mapped_device {
    struct input_dev *dev;
    mapped_key *mapped_keys;
};

static mapped_key parse_ulong(ulong value) {
    mapped_key key;
    key.from = (value >> 32) & 0xFFFFFFFF;
    key.to = value & 0xFFFFFFFF;
    return key;
}

static mapped_key* parse_table(ulong *table, int count) {
    mapped_key *mapped_keys = kmalloc(count * sizeof(mapped_key), GFP_KERNEL);
    for (int i = 0; i < count; i++) {
        mapped_keys[i] = parse_ulong(table[i]);
    }
    return mapped_keys;
}

static int connect_device(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
    pr_info("Device %s connected\n", dev->name);
    return 0;
}

static void disconnect_device(struct input_handle *handle) {
    input_unregister_handle(handle);
    pr_info("Device %s disconnected\n", handle->dev->name);
}

static bool filter_device(struct input_handler *handler, struct input_dev *dev) {
    if (!(dev->evbit[0] & BIT(EV_KEY))) {
        pr_info("Device %s does not support key events\n", dev->name);
        return false;
    }

    for (int i = 0; i < device_names_count; i++) {
        if (strcmp(dev->name, device_names[i]) == 0) {
            pr_info("Device %s matches\n", dev->name);
            return true;
        }
    }

    pr_info("Device %s not found in target devices\n", dev->name);
    return false;
}

static const struct input_device_id device_ids[] = {
    { .driver_info = 13 },
    {}
};

static struct input_handler input_handler = {
    .connect = connect_device,
    .match = filter_device,
    .disconnect = disconnect_device,
    .name = "input_handler",
    .id_table = device_ids,
};

static int __init remap_init(void)
{
    int error;

    if (device_names_count == 0) {
        pr_err("No target devices specified\n");
        return -EINVAL;
    }

    if (device_tables_count == 0) {
        pr_err("No device tables specified\n");
        return -EINVAL;
    }

    if (device_names_count != device_tables_count) {
        pr_err("Number of target devices and device tables must match\n");
        return -EINVAL;
    }

    error = input_register_handler(&input_handler);
    if (error) {
        pr_err("Failed to register input handler\n");
        return error;
    }

    pr_info("Input handler registered\n");

    return 0;
}

static void __exit remap_exit(void)
{
    input_unregister_handler(&input_handler);
    pr_info("Input handler unregistered\n");
}

module_init(remap_init);
module_exit(remap_exit);