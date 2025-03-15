ROOT_DIR = $(CURDIR)

SOURCE_DIR = src
OUTPUT_DIR = build
TARGET = remapper

KERNEL_DIR = /lib/modules/$(shell uname -r)/build

SOURCES = $(SOURCE_DIR)/main.c
HEADERS = $(SOURCE_DIR)/utils.h

# Depends on bin/include bin/*.c and bin/Kbuild
all: $(OUTPUT_DIR)/ $(subst $(SOURCE_DIR),$(OUTPUT_DIR),$(SOURCES)) $(subst $(SOURCE_DIR),$(OUTPUT_DIR),$(HEADERS)) $(OUTPUT_DIR)/Kbuild
	$(MAKE) -C $(KERNEL_DIR) M=$(ROOT_DIR)/$(OUTPUT_DIR) modules

install: all
	sudo $(MAKE) -C $(KERNEL_DIR) M=$(ROOT_DIR)/$(OUTPUT_DIR) modules_install

remove:
	sudo rmmod $(TARGET)

# Create a symlink from src to bin
$(OUTPUT_DIR)/%: $(SOURCE_DIR)/%
	cp $(ROOT_DIR)/$< $@

# Create any needed folder
%/:
	mkdir -p $@

# Generate a Makefile with the needed obj-m and mymodule-objs set
$(OUTPUT_DIR)/Kbuild:
	printf "obj-m += $(TARGET).o\n$(TARGET)-y := $(subst $(TARGET).o,, $(subst .c,.o,$(subst $(SOURCE_DIR)/,,$(SOURCES))))" > $@

clean:
	make -C $(KERNEL_DIR) M=$(ROOT_DIR)/$(OUTPUT_DIR) clean
	rm -rf $(OUTPUT_DIR)