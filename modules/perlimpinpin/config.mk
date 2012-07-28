SRCS = perlimpinpin.c payload/system.c payload/log.c
MODULES = uart

GEN_FILES = payload/room.c payload/room.h
GEN_SRCS = $(filter %.c,$(GEN_FILES))

ifndef ROOM_TRANSACTIONS
$(error ROOM_TRANSACTIONS must be defined and exported in Makefile)
endif

$(eval $(call py_templatize_dir_rule, \
	$(src_dir)/templates, $(GEN_FILES), \
	$(src_dir)/templates/payload/room.py $(ROOM_TRANSACTIONS) \
	))

