
## Configuration

HOST ?= avr
FORMAT ?= ihex
FORMAT_EXTENSION ?= hex
MATH_LIB ?= yes
PRINTF_LEVEL ?= standard
STD ?= gnu11

project_dir ?= .
src_dir ?= $(project_dir)
obj_dir ?= $(project_dir)/build
gen_dir ?= $(obj_dir)/gen


export HOST project_dir obj_dir gen_dir
export MCU


# Try to guess AVARIX_DIR from current Makefile path
ifeq ($(AVARIX_DIR),)
AVARIX_DIR := $(dir $(lastword $(MAKEFILE_LIST)))..
endif
# Use absolute paths, may help editors.
ABS_AVARIX_DIR := $(abspath $(AVARIX_DIR))
export AVARIX_DIR

# include/compute deps files only for build rules
clean_rules := clean clean-project clean-modules clean-builddeps clean-bootloader
ifeq ($(filter $(clean_rules),$(MAKECMDGOALS)),)
export use_deps := yes
endif


## Internal variables

this_makefile := $(lastword $(MAKEFILE_LIST))

# Module dependencies
modules_deps = $(obj_dir)/modules.deps
export modules_deps

-include $(modules_deps)
PROJECT_MODULES_PATHS = $(addprefix modules/,$(PROJECT_MODULES))
MODULES_PATHS = $(sort $(PROJECT_MODULES_PATHS) bootloader/module $(BOOTLOADER_MODULES_PATHS))

SRC_COBJS = $(SRCS:%.c=$(obj_dir)/%.$(HOST).o)
GEN_COBJS = $(GEN_SRCS:%.c=$(obj_dir)/%.$(HOST).o)
COBJS = $(SRC_COBJS) $(GEN_COBJS)
AOBJS = $(ASRCS:%.S=$(obj_dir)/%.$(HOST).o)
OBJS = $(COBJS) $(AOBJS)
DEPS = $(COBJS:.o=.d)
GEN_FILES_FULL = $(addprefix $(gen_dir)/,$(GEN_FILES))
PROJECT_MODULES_LIBS = $(foreach m,$(PROJECT_MODULES_PATHS),$(obj_dir)/$(m).$(HOST).a)
PROJECT_LIB = $(obj_dir)/$(TARGET).$(HOST).a


## Programs and commands

ifeq ($(HOST),avr)
CC = avr-gcc
AR = avr-ar
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
NM = avr-nm
TARGET_OBJ = $(TARGET).elf
OUTPUTS = $(TARGET).$(FORMAT_EXTENSION) $(TARGET).eep
else
CC = gcc
AR = ar
OBJCOPY = objcopy
OBJDUMP = objdump
SIZE = size --format=Berkeley
NM = nm
TARGET_OBJ = $(TARGET)
OUTPUTS = $(TARGET_OBJ)
endif
PY_TEMPLATIZE = $(AVARIX_DIR)/mk/templatize.py
export CC AR PY_TEMPLATIZE

# On Windows, default find.exe is not the POSIX find
ifeq ($(OS),Windows_NT)
ifeq ($(dir $(SHELL)),/bin/)  # assume MSYS or Cygwin
FIND_DIR_NAME = /usr/bin/find $(1) -name $(2)
else
FIND_DIR_NAME = cd $(1) && find /S /B $(2)
endif
else
FIND_DIR_NAME = find $(1) -name $(2)
endif

# Prog
AVRDUDE = avrdude
AVRDUDE_PART = $(patsubst atxmega%,x%,$(MCU))
AVRDUDE_CMD = $(AVRDUDE) -p $(AVRDUDE_PART)

-include $(AVARIX_DIR)/mk/common.mk


## Various flags

INCLUDE_DIRS += . $(ABS_AVARIX_DIR)/include $(ABS_AVARIX_DIR)/modules $(gen_dir)/modules

CPPFLAGS += -Wall $(if $(STD),-std=$(STD))
CPPFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
CFLAGS += -O$(OPT)
CFLAGS += -Wextra -Wno-unused-parameter -Wstrict-prototypes
ASFLAGS +=

ifeq ($(HOST),avr)

CPPFLAGS += -mmcu=$(MCU)
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
ASFLAGS += -Wa,-gstabs
LDFLAGS += -mmcu=$(MCU)

printf_LDFLAGS_minimal := -Wl,-u,vfprintf -lprintf_min
printf_LDFLAGS_standard :=
printf_LDFLAGS_advanced := -Wl,-u,vfprintf -lprintf_flt
PROJECT_LDFLAGS = $(LDFLAGS)
PROJECT_LDFLAGS += $(printf_LDFLAGS_$(PRINTF_LEVEL))

else

CPPFLAGS += -DHOST_VERSION

endif

ifneq ($(filter-out no,$(MATH_LIB)),)
PROJECT_LDFLAGS += -lm
endif

export CPPFLAGS CFLAGS ASFLAGS LDFLAGS


## Rules

default: all

ifneq ($(use_deps),)
-include $(DEPS)
endif

all: $(OUTPUTS)

help:
	@echo ""
	@echo "Special goals:"
	@echo "  all                   build all (default)"
	@echo "  size                  display size of output object"
	@echo "  bootloader            build bootloader"
	@echo "  bootloader-size       display bootloader size"
	@echo "  prog                  program the application"
	@echo "  prog-bootloader       program the bootloader"
	@echo "  prog-bootloader-fuse  program the bootloader fuse if needed"
	@echo "  defaultconf           retrieve default conf file of all modules"
	@echo "  defaultconf-MODULE    retrieve default conf file of module MODULE"
	@echo "  list-modules          list available modules and whether they are enabled"
	@echo "  MODULE                build module MODULE"
	@echo "  clean                 clean all"
	@echo "  clean-project         clean project objects and outputs"
	@echo "  clean-modules         clean all module objects"
	@echo "  clean-MODULE          clean objects for module MODULE"
	@echo "  build/**/*.E          preprocessor output for associated object file"
	@echo "  get-preproc-DEFINE     output C preprocessor definition"
	@echo ""
	@echo "MODULE is a module path, prefixed by 'modules/'."
	@echo "For instance:  modules/comm/uart"
	@echo ""
	@echo "Some configuration variables:"
	@echo "  MCU      target type (-mcu option)"
	@echo "  OPT      optimization level (-O option)"
	@echo "  STD      C standard (-std option)"
	@echo ""


# Modules and their dependencies

$(modules_deps): $(firstword $(MAKEFILE_LIST))
ifneq ($(use_deps),)
	@mkdir -p $(obj_dir) $(gen_dir)
	$(MAKE) -f $(AVARIX_DIR)/mk/moduledeps.mk new_modules='$(MODULES)'
endif

# Dependency template for modules
# The 'true' command is required to rebuild the output on .a change.
#   module_lib_tpl <module-path>
define module_lib_tpl
$(obj_dir)/$(1).$(HOST).a: $(1)
	@true

defaultconf-$(1):
	$(MAKE) -f $(AVARIX_DIR)/mk/module.mk \
		src_dir=$(AVARIX_DIR)/$(1) obj_dir=$(obj_dir)/$(1) gen_dir=$(gen_dir)/$(1) \
		defaultconf $(src_dir)

$(obj_dir)/$(1)/%: .force
	$(MAKE) -f $(AVARIX_DIR)/mk/module.mk \
		src_dir=$(AVARIX_DIR)/$(1) obj_dir=$(obj_dir)/$(1) gen_dir=$(gen_dir)/$(1) \
		$$@

$(gen_dir)/$(1)/%: .force
	$(MAKE) -f $(AVARIX_DIR)/mk/module.mk \
		src_dir=$(AVARIX_DIR)/$(1) obj_dir=$(obj_dir)/$(1) gen_dir=$(gen_dir)/$(1) \
		$$@

endef
$(foreach m,$(MODULES_PATHS),$(eval $(call module_lib_tpl,$m)))

$(MODULES_PATHS):
	$(MAKE) -f $(AVARIX_DIR)/mk/module.mk \
		src_dir=$(AVARIX_DIR)/$@ obj_dir=$(obj_dir)/$@ gen_dir=$(gen_dir)/$@

defaultconf: $(addprefix defaultconf-,$(MODULES_PATHS))


# note: variable is only computed when used, which ensure there is no useless
# search if list-modules is not "called"
modules_list = $(patsubst $(AVARIX_DIR)/modules/%/config.mk,%,$(shell $(call FIND_DIR_NAME,$(AVARIX_DIR)/modules,config.mk)))
define list_modules_rule_tpl
	@echo " $(if $(findstring $(1),$(PROJECT_MODULES)),+, ) $(1)"

endef

list-modules:
	$(foreach m,$(modules_list),$(call list_modules_rule_tpl,$m))


# Project outputs

$(TARGET_OBJ): $(PROJECT_MODULES_LIBS) $(PROJECT_LIB)
	$(CC) $(PROJECT_LIB) $(PROJECT_MODULES_LIBS) -o $@ $(PROJECT_LDFLAGS)

# project objects, with renamed sections
$(PROJECT_LIB): $(OBJS)
	$(AR) rs $@ $(OBJS) 2>/dev/null
	$(OBJCOPY) \
		--rename-section .text=.text.project \
		--rename-section .data=.data.project \
		--rename-section .bss=.bss.project \
		$@

size:
	@$(SIZE) $(TARGET_OBJ)


# Make sure to generate files first
$(OBJS): $(GEN_FILES_FULL)
$(OBJS): $(PROJECT_MODULES_LIBS)

# Usual object files

$(SRC_COBJS): $(obj_dir)/%.$(HOST).o: $(src_dir)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MD -MF $(@:.o=.d) -c $< -o $@

$(GEN_COBJS): $(obj_dir)/%.$(HOST).o: $(gen_dir)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MD -MF $(@:.o=.d) -c $< -o $@

$(AOBJS): $(obj_dir)/%.$(HOST).o: $(src_dir)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

# Preprocessor output

$(SRC_COBJS:.o=.E): $(obj_dir)/%.$(HOST).E: $(src_dir)/%.c .force
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -E $< -o $@

$(GEN_COBJS:.o=.E): $(obj_dir)/%.$(HOST).E: $(gen_dir)/%.c .force
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -E $< -o $@

$(AOBJS:.o=.E): $(obj_dir)/%.$(HOST).E: $(src_dir)/%.S .force
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -E $< -o $@

# Get greprocessor definition

get-preproc-%:
	( for x in avr/io.h $(GET_PREPROC_INCLUDES) ; do \
		  echo '#include <'$$x'>' ; \
		done ; \
		echo '$*' ; \
	) | $(CC) $(CPPFLAGS) -E -x c - | tail -n1

# Other (final) outputs (.hex, .eep) from ELF

%.$(FORMAT_EXTENSION): %.elf
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

# ignore error if eeprom is not used
%.eep: %.elf
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
		--change-section-lma .eeprom=0 --no-change-warnings \
		-O $(FORMAT) $< $@


# Add bootloader rules
-include $(AVARIX_DIR)/mk/bootloader.mk


# Programming

prog: $(OUTPUTS)
	$(AVRDUDE_CMD) \
		-U flash:w:$(TARGET).$(FORMAT_EXTENSION) \
		-U eeprom:w:$(TARGET).eep


# Cleaning

clean: clean-project clean-modules clean-bootloader
	-rmdir -p $(sort $(obj_dir) $(gen_dir) $(dir $(OBJS) $(GEN_FILES_FULL))) 2>/dev/null

clean-project:
	rm -f $(TARGET_OBJ) $(PROJECT_LIB) $(OUTPUTS) \
		$(GEN_FILES_FULL) $(OBJS) $(OBJS:.o=.E) $(DEPS) $(modules_deps)

clean_modules_rules = $(addprefix clean-,$(MODULES_PATHS))

clean-modules: $(clean_modules_rules)


$(clean_modules_rules): clean-%:
	@$(MAKE) src_dir=$(AVARIX_DIR)/$* obj_dir=$(obj_dir)/$* gen_dir=$(gen_dir)/$* \
		-f $(AVARIX_DIR)/mk/module.mk clean


.PHONY : $(clean_rules) $(clean_modules_rules) \
	size help list-modules .force

