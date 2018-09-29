
## Configuration

# The following variables must be set by the caller
#   src_dir
#   obj_dir
#   gen_dir
#   HOST
.DEFAULT_GOAL = default
-include $(AVARIX_DIR)/mk/common.mk
include $(src_dir)/config.mk

project_dir ?= .
CONFIGS ?= $(addprefix config/,$(notdir $(wildcard $(src_dir)/config/*_config.h)))

# Internal variables
TARGET_OBJ = $(obj_dir).$(HOST).a
SRC_COBJS = $(SRCS:%.c=$(obj_dir)/%.$(HOST).o)
GEN_COBJS = $(GEN_SRCS:%.c=$(obj_dir)/%.$(HOST).o)
COBJS = $(SRC_COBJS) $(GEN_COBJS)
AOBJS = $(ASRCS:%.S=$(obj_dir)/%.$(HOST).o)
OBJS = $(COBJS) $(AOBJS)
DEPS = $(COBJS:.o=.d)
GEN_FILES_FULL = $(addprefix $(gen_dir)/,$(GEN_FILES))


## Rules

default: all

ifneq ($(use_deps),)
-include $(DEPS)
endif

all: $(TARGET_OBJ)

# Make sure to generate files first
$(OBJS): $(GEN_FILES_FULL)

# Module library file
$(TARGET_OBJ): $(OBJS)
	$(AR) rs $@ $(COBJS) 2>/dev/null
ifneq ($(use_avarix_sections),)
	$(OBJCOPY) --prefix-alloc-sections .avarix.module $@
endif

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


# Default configuration

defaultconf_rules = $(addprefix defaultconf-,$(CONFIGS))

defaultconf: $(defaultconf_rules)

$(defaultconf_rules): defaultconf-%: $(src_dir)/%
	-[ -f $(project_dir)/$(notdir $<) ] || cp $< $(project_dir)/

clean:
	rm -f $(TARGET_OBJ) $(GEN_FILES_FULL) $(OBJS) $(OBJS:.o=.E) $(DEPS)
	@-rmdir -p $(sort $(obj_dir) $(gen_dir) $(dir $(OBJS) $(GEN_FILES_FULL))) 2>/dev/null


.PHONY : clean .force

