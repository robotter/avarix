
## Compute module dependencies

# Module dependency tree is traversed using a breadth-first search.
# Each module is "checked" only once.
# The makefile script invokes itself recursively. The number of recursive calls
# is at most equal to the length of the longest depedency path.

# The following variables must be set by the caller
#   obj_dir
#   modules_deps
# Each goal is a module name.

new_modules := $(MAKECMDGOALS)
all_modules += $(new_modules)
export all_modules


# $(append_module_deps module)
define append_module_deps
include $(AVARIX_DIR)/modules/$(1)/config.mk
next_modules += $$(MODULES)
endef
next_modules :=
$(foreach m,$(new_modules),$(eval $(call append_module_deps,$m)))

# remove duplicates and already processed modules
next_modules := $(strip $(filter-out $(all_modules),$(sort $(next_modules))))


ifeq ($(new_modules),)

# no more modules
default:
	@echo 'ALL_MODULES = $(all_modules)' > $(modules_deps)

else

default:
	@$(MAKE) -f $(AVARIX_DIR)/mk/moduledeps.mk \
		new_modules='$(next_modules)'

endif

