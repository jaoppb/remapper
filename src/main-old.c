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
static char* device_tables[MAX_DEVICES];

module_param_array(device_names, charp, &device_names_count, 0);
MODULE_PARM_DESC(device_names, "Path to the device to be remapped");

module_param_array(device_tables, charp, &device_tables_count, 0);
MODULE_PARM_DESC(device_tables, "Table of key mappings");

typedef struct {
    __u8* from;
    size_t from_size;
    keycode to;
} mapped_key;

struct mapped_device {
    struct input_dev *dev;
    mapped_key *mapped_keys;
};

static int parse_remap(mapped_key* key, char *from, keycode to) {
    char* each = NULL;
    __u8* result = kcalloc(32, sizeof(__u8), GFP_KERNEL);

    if (result == NULL) {
        pr_err("Failed to allocate memory for key\n");
        return -1;
    }

    int size = 0;
    while ((each = strsep(&from, " ")) != NULL) {
        if (size >= 32) {
            pr_err("Key too long\n");
            return -1;
        }

        kstrtou8(each, 16, &result[size]);

        size++;
    }

    key->from = kcalloc(size, sizeof(__u8), GFP_KERNEL);
    if (key->from == NULL) {
        pr_err("Failed to allocate memory for key\n");
        return -1;
    }

    key->from_size = size;
    memcpy(key->from, result, size);
    kfree(result);

    key->to = to;
    return 0;
}

static int parse_table(mapped_key** mapped_keys, char* table, int count) {
    for (int i = 0; i < count; i++) {
        if(parse_remap(mapped_keys[i], strsep(&table, ";"), table[0]) < 0) {
            return -1;
        }
    }
    return 0;
}

static int remap_device(struct input_dev *dev, mapped_key* m_key) {
    struct input_keymap_entry keymap = {
        .keycode = m_key->to,
        .len = m_key->from_size,
    };
    memcpy(&keymap.scancode, m_key->from, m_key->from_size);

    if (input_set_keycode(dev, &keymap) < 0) {
        pr_err("Failed to remap key\n");
        return -1;
    }

    return 0;
}

static int get_device_index(const char* name) {
    for (int i = 0; i < device_names_count; i++) {
        if (strcmp(name, device_names[i]) == 0) {
            return i;
        }
    }

    return -1;
}

static int connect_device(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {   
    int index = get_device_index(dev->name);
    if (index < 0) {
        return -1;
    }

    struct mapped_device *mapped_dev = kzalloc(sizeof(struct mapped_device), GFP_KERNEL);
    if (mapped_dev == NULL) {
        pr_err("Failed to allocate memory for mapped device\n");
        return -ENOMEM;
    }

    mapped_dev->dev = dev;
    mapped_dev->mapped_keys = kcalloc(32, sizeof(mapped_key), GFP_KERNEL);
    
    if (mapped_dev->mapped_keys == NULL) {
        pr_err("Failed to allocate memory for mapped keys\n");
        kfree(mapped_dev);
        return -ENOMEM;
    }

    if (parse_table(&mapped_dev->mapped_keys, device_tables[index], 32) < 0) {
        pr_err("Failed to parse key table\n");
        kfree(mapped_dev->mapped_keys);
        kfree(mapped_dev);
        return -1;
    }

    if (remap_device(dev, mapped_dev->mapped_keys) < 0) {
        pr_err("Failed to remap key\n");
        kfree(mapped_dev->mapped_keys);
        kfree(mapped_dev);
        return -1;
    }

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

    int index = get_device_index(dev->name);
    if (index < 0) {
        pr_info("Device %s is not in the list of target devices\n", dev->name);
        return false;
    }

    return true;
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