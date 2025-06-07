ROOT_DIR = $(CURDIR)

SRCDIR = src
DESTDIR = build
TARGET = remapper

KERNEL_DIR = /lib/modules/$(KERNEL_UNAME)/build

SOURCES = $(SRCDIR)/main.c
HEADERS = $(SRCDIR)/utils.h

# Depends on bin/include bin/*.c and bin/Kbuild
all: $(DESTDIR)/ $(subst $(SRCDIR),$(DESTDIR),$(SOURCES)) $(subst $(SRCDIR),$(DESTDIR),$(HEADERS)) $(DESTDIR)/Kbuild
	$(MAKE) -C $(KERNEL_DIR) M=$(ROOT_DIR)/$(DESTDIR) modules

install: all
	$(MAKE) -C $(KERNEL_DIR) M=$(ROOT_DIR)/$(DESTDIR) modules_install

remove:
	rmmod $(TARGET)

# Create a symlink from src to bin
$(DESTDIR)/%: $(SRCDIR)/%
	cp $(ROOT_DIR)/$< $@

dkms: install
	cp $(DESTDIR)/*.ko .

# Create any needed folder
%/:
	mkdir -p $@

# Generate a Makefile with the needed obj-m and mymodule-objs set
$(DESTDIR)/Kbuild:
	printf "obj-m += $(TARGET).o\n$(TARGET)-y := $(subst $(TARGET).o,, $(subst .c,.o,$(subst $(SRCDIR)/,,$(SOURCES))))" > $@

clean:
	make -C $(KERNEL_DIR) M=$(ROOT_DIR)/$(DESTDIR) clean
	rm -rf $(DESTDIR)
