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
static char key_table[MAX_REMAPS];

module_param_array(key_table, charp, &key_table_count, 0);
MODULE_PARM_DESC(key_table, "Table of key mappings");

// Input Handler methods

static bool filter_device(struct input_handler *handler, struct input_dev *dev) {
    return strcmp(dev->name, device_name) == 0;
}

static int connect_device(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
    pr_info("Device %s connected\n", dev->name);
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
    if (input_register_handler(&input_handler) < 0) {
        pr_err("Failed to register input handler\n");
        return -1;
    }

    pr_info("Input handler registered\n");

    return 0;
}

static void __exit remap_exit(void) {
    input_unregister_handler(&input_handler);
    pr_info("Input handler unregistered\n");
}

module_init(remap_init);
module_exit(remap_exit);