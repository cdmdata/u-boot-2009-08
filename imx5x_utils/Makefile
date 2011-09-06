
include $(TOPDIR)/config.mk

LIB	= $(obj)libbd.a


COBJS_MAIN := main_ecspi.o main_ecspi_write.o main_ecspi_ram_write.o main_sdmmc.o main_sdmmc_write.o main_ddr_init.c
SOBJS_MAIN := mx5x_fuses.o$(SOC)_ecspi mx5x_fuses.o$(SOC) mx5x_base.o$(SOC)  
COBJS	:= mx5x_common.o mx5x_ecspi.o mx5x_i2c.o mx5x_sdmmc.o $(SOC)_header.o adler32.o $(SOC)_ddr_calibrate.o
SOBJS	:= _udivsi3.o xmodemReceive.o

SRCS	:= $(SOBJS:.o=.S) mx5x_fuses.S mx5x_base.S $(COBJS:.o=.c) $(COBJS_MAIN:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))
SOBJS	:= $(addprefix $(obj),$(SOBJS)) ../lib_arm/$(obj)_umodsi3.o

OBJS_MAIN := $(addprefix $(obj),$(COBJS_MAIN))
SOBJS_MAIN := $(addprefix $(obj),$(SOBJS_MAIN))

CFLAGS += -ffunction-sections -fdata-sections -mabi=apcs-gnu

SOC_1_FILES := $(obj)$(SOC)_fuses_ecspi.bin $(obj)$(SOC)_fuses_sdmmc.bin
SOC_2_FILES := $(obj)$(SOC)_ecspi.bin $(obj)$(SOC)_sdmmc.bin
SOC_3_FILES := $(obj)$(SOC)_ecspi_xm.bin $(obj)$(SOC)_ecspi_write.bin $(SOC)_ecspi_ram_write_xm.bin $(SOC)_ddr_init_xm.bin $(obj)$(SOC)_ecspi_clearenv.bin
SOC_3_FILES += $(obj)$(SOC)_sdmmc_xm.bin $(obj)$(SOC)_sdmmc_write.bin
SOC_3_FILES += $(obj)$(SOC)_ddr_calibrate.bin

TARGETS := $(LIB) $(SOC_1_FILES) $(SOC_2_FILES) $(SOC_3_FILES)
all:    $(TARGETS)

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS) $(SOBJS)

clean:
	rm -f $(SOBJS) $(OBJS) $(SOBJS_MAIN) $(OBJS_MAIN) $(TARGETS) \
		$(SOC_1_FILES:.bin=.o$(SOC)) $(SOC_1_FILES:.bin=.o$(SOC_ecspi)) \
		$(SOC_1_FILES:.bin=.o$(SOC)_1) $(SOC_2_FILES:.bin=.o$(SOC)_2) $(SOC_3_FILES:.bin=.o$(SOC)_3) \
		$(SOC_1_FILES:.bin=.map) $(SOC_2_FILES:.bin=.map) $(SOC_3_FILES:.bin=.map)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

$(obj)%.o$(SOC): %.S
	$(CC) -c $(CFLAGS) -D$(SOC) -o $@ $<

$(obj)%.o$(SOC)_ecspi: %.S
	$(CC) -c $(CFLAGS) -D$(SOC) -DECSPI_BURN -o $@ $<

$(obj)%.omx51_1:
	$(LD) --defsym=hdr_offset=0x0 --defsym=load_addr=0x1ffe4000 -static --no-warn-mismatch -nostdlib -T $(filter %.lds,$^) $(filter-out %.lds,$^) -o $@ -M >$(obj)$*.map

$(obj)%.omx53_1:
	$(LD) --defsym=hdr_offset=0x0 --defsym=load_addr=0xf8006000 -static --no-warn-mismatch -nostdlib -T $(filter %.lds,$^) $(filter-out %.lds,$^) -o $@ -M >$(obj)$*.map

# "hdr_offset" = 0x400 to load from SDCard, 0x0 to download over serial */

$(obj)%.omx51_2:
	$(LD) --defsym=hdr_offset=0x400 --defsym=load_addr=0x1ffe2000 -static --no-warn-mismatch -nostdlib -T $(filter %.lds,$^) $(filter-out %.lds,$^) --gc-sections $(LIB) -o $@ -M >$(obj)$*.map

$(obj)%.omx53_2:
	$(LD) --defsym=hdr_offset=0x400 --defsym=load_addr=0xf8006000 -static --no-warn-mismatch -nostdlib -T $(filter %.lds,$^) $(filter-out %.lds,$^) --gc-sections $(LIB) -o $@ -M >$(obj)$*.map

$(obj)%.omx51_3:
	$(LD) --defsym=hdr_offset=0x0 --defsym=load_addr=0x1ffe4000 -static --no-warn-mismatch -nostdlib -T $(filter %.lds,$^) $(filter-out %.lds,$^) --gc-sections $(LIB) -o $@ -M >$(obj)$*.map

$(obj)%.omx53_3:
	$(LD) --defsym=hdr_offset=0x0 --defsym=load_addr=0xf8006000 -static --no-warn-mismatch -nostdlib -T $(filter %.lds,$^) $(filter-out %.lds,$^) --gc-sections $(LIB) -o $@ -M >$(obj)$*.map

$(obj)$(SOC)_ecspi.o$(SOC)_2 $(obj)$(SOC)_ecspi_xm.o$(SOC)_3:  mx5x.lds $(obj)mx5x_base.o$(SOC) $(obj)main_ecspi.o $(Lib)

$(obj)$(SOC)_sdmmc.o$(SOC)_2 $(obj)$(SOC)_sdmmc_xm.o$(SOC)_3:  mx5x.lds $(obj)mx5x_base.o$(SOC) $(obj)main_sdmmc.o $(Lib)

$(obj)$(SOC)_ecspi_write.o$(SOC)_3:  mx5x.lds $(obj)mx5x_base.o$(SOC) $(obj)main_ecspi_write.o $(Lib)

$(obj)$(SOC)_ecspi_ram_write_xm.o$(SOC)_3:  mx5x.lds $(obj)mx5x_base.o$(SOC) $(obj)main_ecspi_ram_write.o $(Lib)

$(obj)$(SOC)_ddr_init_xm.o$(SOC)_3:  mx5x.lds $(obj)mx5x_base.o$(SOC) $(obj)main_ddr_init.o $(Lib)

$(obj)$(SOC)_ecspi_clearenv.o$(SOC)_3:  mx5x.lds $(obj)mx5x_base.o$(SOC) $(obj)main_ecspi_clearenv.o $(Lib)

$(obj)$(SOC)_sdmmc_write.o$(SOC)_3:  mx5x.lds $(obj)mx5x_base.o$(SOC) $(obj)main_sdmmc_write.o $(Lib)

$(obj)$(SOC)_ddr_calibrate.o$(SOC)_3:  mx5x.lds $(obj)mx5x_base.o$(SOC) $(obj)$(SOC)_ddr_calibrate.o $(Lib)

$(obj)$(SOC)_fuses_sdmmc.o$(SOC)_1: mx5x.lds $(obj)mx5x_fuses.o$(SOC)

$(obj)$(SOC)_fuses_ecspi.o$(SOC)_1: mx5x.lds $(obj)mx5x_fuses.o$(SOC)_ecspi

$(SOC_1_FILES) : $(obj)%.bin : $(obj)%.o$(SOC)_1
	$(OBJCOPY) -O binary --gap-fill 0xff $< $@

$(SOC_2_FILES) : $(obj)%.bin : $(obj)%.o$(SOC)_2
	$(OBJCOPY) -O binary --gap-fill 0xff $< $@

$(SOC_3_FILES) : $(obj)%.bin : $(obj)%.o$(SOC)_3
	$(OBJCOPY) -O binary --gap-fill 0xff $< $@

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
	