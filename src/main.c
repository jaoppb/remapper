#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/input.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jaoppb");
MODULE_DESCRIPTION("A simple input remapper");

static int connect_device(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
    pr_info("Device connected\n");
    return 0;
}

static void disconnect_device(struct input_handle *handle) {
    pr_info("Device disconnected\n");
}

static const struct input_device_id device_ids[] = {
    { .driver_info = 13 },
    {}
};

static struct input_handler input_handler = {
    .connect = connect_device,
    .disconnect = disconnect_device,
    .name = "input_handler",
    .id_table = device_ids,
};

static int __init remap_init(void)
{
    int error;

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