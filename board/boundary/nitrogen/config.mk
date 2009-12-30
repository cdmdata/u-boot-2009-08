LDSCRIPT := $(SRCTREE)/board/$(VENDOR)/$(BOARD)/u-boot.lds

sinclude $(OBJTREE)/board/$(VENDOR)/$(BOARD)/config.tmp

ifndef TEXT_BASE
	TEXT_BASE = 0x93f00000
endif
CONFIG_HW_WATCHDOG=y
