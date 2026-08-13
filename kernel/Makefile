# ─────────────────────────────────────────────────────────────────────────
# Kbuild Makefile for High-Performance Gateway Linux Kernel Module (LKM)
# ─────────────────────────────────────────────────────────────────────────

obj-m += gateway_stats_mod.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod gateway_stats_mod.ko
	dmesg | tail -n 5

unload:
	sudo rmmod gateway_stats_mod
	dmesg | tail -n 5
