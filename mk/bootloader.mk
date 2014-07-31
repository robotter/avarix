
# This script is intended to be included from project.mk

## Bootloader configuration

BOOTLOADER_TARGET = bootloader
# module dependencies must be flatten here
BOOTLOADER_MODULES = clock


## Internal variables

BOOTLOADER_OUTPUT = $(BOOTLOADER_TARGET).$(FORMAT_EXTENSION)
BOOTLOADER_TARGET_OBJ = $(BOOTLOADER_TARGET).elf
BOOTLOADER_OBJS = $(BOOTLOADER_SRCS:%.c=$(obj_dir)/bootloader/%.$(HOST).o)
BOOTLOADER_MODULES_PATHS = bootloader/module $(addprefix modules/,$(BOOTLOADER_MODULES))
BOOTLOADER_MODULES_LIBS = $(foreach m,$(BOOTLOADER_MODULES_PATHS),$(obj_dir)/$(m).$(HOST).a)

get_bootloader_addr = $$(printf '0x%x' $$(($$($(MAKE) -s get-preproc-BOOT_SECTION_START))))
BOOTLOADER_LDFLAGS = $(LDFLAGS)
BOOTLOADER_LDFLAGS += -Wl,--section-start=.text=$(get_bootloader_addr)


# Targets

bootloader: $(BOOTLOADER_OUTPUT)

bootloader-size:
	@$(SIZE) $(BOOTLOADER_TARGET_OBJ)

$(BOOTLOADER_OUTPUT): $(BOOTLOADER_TARGET_OBJ)
	$(OBJCOPY) -O $(FORMAT) --change-addresses -$(get_bootloader_addr) -R .eeprom $< $@

$(BOOTLOADER_TARGET_OBJ): $(BOOTLOADER_MODULES_LIBS)
	$(CC) $(BOOTLOADER_MODULES_LIBS) -o $@ $(BOOTLOADER_LDFLAGS)


# Programming
# note: BOOTRST is in FUSEBYTE2, bit 6

bootrst_fusebyte = fuse2
bootrst_bit = 6

prog-bootloader: bootloader
	$(AVRDUDE_CMD) -U boot:w:$(BOOTLOADER_OUTPUT)
	@(( $$($(AVRDUDE_CMD) -qq -U $(bootrst_fusebyte):r:-:d) & (1<<$(bootrst_bit)) )) && \
		echo "BOOTRST fuse bit not configured, run 'make prog-bootloader-fuse'."

prog-bootloader-fuse:
	b=$$($(AVRDUDE_CMD) -qq -U $(bootrst_fusebyte):r:-:d) ; \
	if (( b & (1<<$(bootrst_bit)) )); then \
		$(AVRDUDE_CMD) -U $(bootrst_fusebyte):w:$$(( b & ~(1<<$(bootrst_bit)) )):d ; \
		fi


# Cleaning

clean-bootloader: $(addprefix clean-,$(BOOTLOADER_MODULES_PATHS))
	rm -f $(BOOTLOADER_OUTPUT) $(BOOTLOADER_TARGET_OBJ)

