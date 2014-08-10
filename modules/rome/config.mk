SRCS = rome.c
MODULES = uart timer

GEN_FILES = rome_msg.h

$(eval $(call py_templatize_rule, \
	$(src_dir)/rome_msg.tpl.h, rome_msg.h, \
	$(src_dir)/rome_msg.py $(ROME_MESSAGES), \
	$(src_dir)/rome_msg.py $(ROME_MESSAGES) \
	))

