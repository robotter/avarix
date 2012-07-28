
## Configuration

# The following variables must be set by the caller
#   src_dir
#   obj_dir
#   HOST
include $(src_dir)/config.mk

project_dir ?= .
CONFIGS ?= $(addprefix config/,$(notdir $(wildcard $(src_dir)/config/*_config.h)))

# Internal variables
TARGET_OBJ = $(obj_dir).$(HOST).a
COBJS = $(SRCS:%.c=$(obj_dir)/%.$(HOST).o)
AOBJS = $(ASRCS:%.S=$(obj_dir)/%.$(HOST).o)
OBJS = $(COBJS) $(AOBJS)
DEPS = $(COBJS:.o=.d)


## Rules

default: all

ifneq ($(use_deps),)
-include $(DEPS)
endif

all: $(TARGET_OBJ)

# Module library file
$(TARGET_OBJ): $(OBJS)
	$(AR) rs $@ $(COBJS) 2>/dev/null

$(COBJS): $(obj_dir)/%.$(HOST).o: $(src_dir)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MD -MF $(@:.o=.d) -c $< -o $@

$(AOBJS): $(obj_dir)/%.$(HOST).o: $(src_dir)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@


# Default configuration

defaultconf_rules = $(addprefix defaultconf-,$(CONFIGS))

defaultconf: $(defaultconf_rules)

$(defaultconf_rules): defaultconf-%: $(src_dir)/%
	-[ -f $(project_dir)/$(notdir $<) ] || cp $< $(project_dir)/

clean:
	rm -f $(TARGET_OBJ) $(OBJS) $(DEPS)
	@-rmdir -p $(sort $(obj_dir) $(dir $(OBJS))) 2>/dev/null


.PHONY : clean

