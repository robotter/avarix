
## Common definitions
#
# This file must not define rules itself.


# Define rules to generated a single template using PY_TEMPLATIZE
#   py_templatize_rule <template> <output-name> [args] [deps]
define py_templatize_rule
$$(gen_dir)/$(strip $(2)): $(1) $(4)
	@mkdir -p $$(dir $$@)
	$(PY_TEMPLATIZE) -o $$@ -i $$< -- $(3)
endef

# Define rule to generate templates from a directory using PY_TEMPLATIZE
#   py_templatize_dir_rule <template-dir> <output-names> [args] [deps]
define py_templatize_dir_rule
$$(addprefix $$(gen_dir)/,$(2)): $$(gen_dir)/%: $(1)/% $(4)
	@mkdir -p $$(dir $$@)
	$(PY_TEMPLATIZE) -o $$@ -i $$< -- $(3)
endef

