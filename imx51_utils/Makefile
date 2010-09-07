
include $(TOPDIR)/config.mk

LIB	= $(obj)libbd.a


COBJS_MAIN := main_ecspi.o main_ecspi_write.o main_sdmmc.o main_sdmmc_write.o
SOBJS_MAIN := mx51_fuses_ecspi.o mx51_fuses_sdmmc.o mx51_base.o  
COBJS	:= mx51_common.o mx51_ecspi.o mx51_sdmmc.o
SOBJS	:= _udivsi3.o xmodemReceive.o

SRCS	:= $(SOBJS:.o=.S) mx51_fuses.S mx51_base.S $(COBJS:.o=.c) $(COBJS_MAIN:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

OBJS_MAIN := $(addprefix $(obj),$(COBJS_MAIN))
SOBJS_MAIN := $(addprefix $(obj),$(SOBJS_MAIN))

CFLAGS += -ffunction-sections -fdata-sections -mabi=apcs-gnu

BINS1 := $(obj)mx51_fuses_ecspi.bin $(obj)mx51_fuses_sdmmc.bin
BINS3 := $(obj)mx51_ecspi.bin $(obj)mx51_sdmmc.bin
BINS4 := $(obj)mx51_ecspi_xm.bin $(obj)mx51_ecspi_write.bin 
BINS4 += $(obj)mx51_sdmmc_xm.bin $(obj)mx51_sdmmc_write.bin
TARGETS := $(LIB) $(BINS1) $(BINS3) $(BINS4)

all:    $(TARGETS)

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS) $(SOBJS)

clean:
	rm -f $(SOBJS) $(OBJS) $(SOBJS_MAIN) $(OBJS_MAIN) $(TARGETS) $(BINS1:.bin=.o1) $(BINS3:.bin=.o3) $(BINS4:.bin=.o4) $(BINS1:.bin=.map) $(BINS3:.bin=.map) $(BINS4:.bin=.map)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

$(obj)%.o1:
	$(LD) --defsym=hdr_offset=0x0 --defsym=load_addr=0x1ffe4000 -static --no-warn-mismatch -nostdlib -T $(filter %.lds,$^) $(filter-out %.lds,$^) -o $@ -M >$(obj)$*.map

# "hdr_offset" = 0x400 to load from SDCard, 0x0 to download over serial */

$(obj)%.o3:
	$(LD) --defsym=hdr_offset=0x400 --defsym=load_addr=0x1ffe2000 -static --no-warn-mismatch -nostdlib -T $(filter %.lds,$^) $(filter-out %.lds,$^) --gc-sections $(LIB) -o $@ -M >$(obj)$*.map

$(obj)%.o4:
	$(LD) --defsym=hdr_offset=0x0 --defsym=load_addr=0x1ffe4000 -static --no-warn-mismatch -nostdlib -T $(filter %.lds,$^) $(filter-out %.lds,$^) --gc-sections $(LIB) -o $@ -M >$(obj)$*.map

$(obj)mx51_fuses_sdmmc.o : $(obj)%_sdmmc.o : %.S
	$(CC) -c $(CFLAGS) -o $@ $<

$(obj)mx51_fuses_ecspi.o : $(obj)%_ecspi.o : %.S
	$(CC) -c $(CFLAGS) -DECSPI_BURN -o $@ $<

$(obj)mx51_ecspi.o3 $(obj)mx51_ecspi_xm.o4:  mx51.lds $(obj)mx51_base.o $(obj)main_ecspi.o $(Lib)

$(obj)mx51_sdmmc.o3 $(obj)mx51_sdmmc_xm.o4:  mx51.lds $(obj)mx51_base.o $(obj)main_sdmmc.o $(Lib)

$(obj)mx51_ecspi_write.o4:  mx51.lds $(obj)mx51_base.o $(obj)main_ecspi_write.o $(Lib)

$(obj)mx51_sdmmc_write.o4:  mx51.lds $(obj)mx51_base.o $(obj)main_sdmmc_write.o $(Lib)

$(obj)mx51_fuses_sdmmc.o1 $(obj)mx51_fuses_ecspi.o1: $(obj)%.o1: mx51.lds $(obj)%.o

$(obj)mx51_ecspi.bin $(obj)mx51_sdmmc.bin : %.bin : $(obj)%.o3
	$(OBJCOPY) -O binary --gap-fill 0xff $< $@

$(obj)mx51_ecspi_write.bin $(obj)mx51_sdmmc_write.bin $(obj)mx51_ecspi_xm.bin $(obj)mx51_sdmmc_xm.bin : %.bin : $(obj)%.o4
	$(OBJCOPY) -O binary --gap-fill 0xff $< $@

$(obj)mx51_fuses_sdmmc.bin $(obj)mx51_fuses_ecspi.bin : %.bin : $(obj)%.o1
	$(OBJCOPY) -O binary --gap-fill 0xff $< $@

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
	