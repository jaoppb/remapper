obj-m += remapper.o
remapper-y = main.c
 
PWD := $(CURDIR)
BUILD_DIR = $(PWD)/build

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

all: $(BUILD_DIR)
	$(MAKE) -C /lib/modules/$(uname -r)/build M=$(PWD) modules 

clean:
	$(MAKE) -C /lib/modules/$(uname -r)/build M=$(PWD) clean