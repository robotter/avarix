SRCS = idle.c
CONFIGS = config/idle_config.py

GEN_FILES = idle_tasks.inc.c idle_tasks.h

$(eval $(call py_templatize_rule, \
	$(src_dir)/idle_tasks.tpl.c, idle_tasks.inc.c, \
	$(src_dir)/idle_tasks.py idle_config.py, \
	$(src_dir)/idle_tasks.py idle_config.py \
	))

$(eval $(call py_templatize_rule, \
	$(src_dir)/idle_tasks.tpl.h, idle_tasks.h, \
	$(src_dir)/idle_tasks.py idle_config.py, \
	$(src_dir)/idle_tasks.py idle_config.py \
	))

