SRCS = rome.c
MODULES = uart timer

GEN_FILES = rome_msg.h

ifeq ($(ROME_MESSAGES),)
rome_msg_deps = $(shell python -c 'import rome_messages as m; print(m.__file__.replace(".pyc",".py"))')
else
rome_msg_deps = $(ROME_MESSAGES)
endif

$(eval $(call py_templatize_rule, \
	$(src_dir)/rome_msg.tpl.h, rome_msg.h, \
	$(src_dir)/rome_msg.py $(ROME_MESSAGES), \
	$(src_dir)/rome_msg.py $(rome_msg_deps) \
	))

