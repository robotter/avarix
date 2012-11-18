
## Project configuration

SRCS = $(wildcard *.c)
ASRCS =
TARGET = main
MODULES = uart
GEN_FILES =
GEN_SRCS = $(filter %.c,$(GEN_FILES))


## Target configuration

MCU = atxmega128a1
F_CPU = 32000000


## Build configuration

OPT = 0
# link against math library (may be implied by some modules)
MATH_LIB = yes
# printf level: minimal, standard, advanced
PRINTF_LEVEL = standard


include ../../mk/project.mk

