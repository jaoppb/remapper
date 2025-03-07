#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/input.h>
#include <linux/printk.h>

static void handle_event(struct input_handle *handle, unsigned int type, unsigned int code, int value) {
    pr_info("Event: type=%d, code=%d, value=%d\n", type, code, value);
}

static const struct input_device_id device_ids[] = {
    { .driver_info = 13 },
    {}
};

static struct input_handler input_handler = {
    .event = handle_event,
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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jaoppb");
MODULE_DESCRIPTION("A simple input remapper");