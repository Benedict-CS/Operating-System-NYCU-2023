obj-m += kfetch_mod_312551002.o
DEVICE_NAME := kfetch_mod_312551002

all: build load create_device_node test

build:
	sudo apt update
	sudo apt install gcc
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

load:
	@if lsmod | grep -q $(DEVICE_NAME); then \
	    echo "Module $(DEVICE_NAME) is already loaded."; \
	else \
	    sudo insmod kfetch_mod_312551002.ko; \
	fi

create_device_node:
	@MAJOR_NUMBER=$(shell grep 'kfetch_mod' /proc/devices | cut -d ' ' -f 1); \
	if [ -z "$$MAJOR_NUMBER" ]; then \
	    echo "Error: Unable to find major number for kfetch_mod."; \
	    exit 1; \
	fi; \
	if [ ! -e /dev/kfetch_mod_312551002 ]; then \
	    sudo mknod /dev/kfetch_mod_312551002 c $$MAJOR_NUMBER 0; \
	    sudo chmod 666 /dev/kfetch_mod_312551002; \
	else \
	    echo "/dev/kfetch_mod_312551002 already exists."; \
	fi

test:
	cc ./kfetch.c -o ./kfetch
	sudo ./kfetch -h
	sudo ./kfetch -a
	sudo ./kfetch -m -c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

unload:
	@if lsmod | grep -q $(DEVICE_NAME); then \
	    sudo rmmod kfetch_mod_312551002; \
	else \
	    echo "Module kfetch_mod_312551002 is not loaded."; \
	fi

remove_device_node:
	@if [ -e /dev/$(DEVICE_NAME) ]; then \
	    sudo rm -f /dev/$(DEVICE_NAME); \
	else \
	    echo "/dev/$(DEVICE_NAME) does not exist."; \
	fi

.PHONY: all build load create_device_node clean unload remove_device_node
