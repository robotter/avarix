SRCS = pathfinding.c
CONFIGS = config/pathfinding_config.py

GEN_FILES = pathfinding_graphs.inc.c pathfinding_graphs.h


$(gen_dir)/pathfinding_graphs.c: $(src_dir)/pathfinding_graphs.py pathfinding_config.py
	$(src_dir)/pathfinding_graphs.py c

$(eval $(call py_templatize_rule, \
	$(src_dir)/pathfinding_graphs.tpl.c, pathfinding_graphs.inc.c, \
	$(src_dir)/pathfinding_graphs.py pathfinding_config.py, \
	$(src_dir)/pathfinding_graphs.py pathfinding_config.py \
	))

$(eval $(call py_templatize_rule, \
	$(src_dir)/pathfinding_graphs.tpl.h, pathfinding_graphs.h, \
	$(src_dir)/pathfinding_graphs.py pathfinding_config.py, \
	$(src_dir)/pathfinding_graphs.py pathfinding_config.py \
	))

