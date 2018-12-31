include ../../py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE = xc32-

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
CPU = 32MX370F512H

CFLAGS_CORTEX_M4 = -MMD -g -mprocessor=$(CPU) -c -fsingle-precision-constant -Wdouble-promotion
CFLAGS = $(INC) -Wall -Werror -std=c99 $(CFLAGS_CORTEX_M4) $(COPT)

#Debugging/Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -O1 -DNDEBUG
endif

LDFLAGS =  -mprocessor=$(CPU) -Wl,--gc-sections,-Map=$@.map,--cref,--report-mem
LIBS = ps2keyboard370f.X.a\

SRC_LIB = $(addprefix lib/,\
	libc/string0.c \
	mp-readline/readline.c \
	netutils/netutils.c \
	timeutils/timeutils.c \
	utils/pyexec.c \
	)

SRC_C = \
	main.c \
	lib_video_megalopa.c\
	text_graph_library.c\
#	printf.c \
	string0.c \
	malloc0.c \
	gccollect.c \

SRC_S = \
	gc.s 

OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_LIB:.c=.o) $(SRC_C:.c=.o) $(SRC_S:.s=.o))

all: $(BUILD)/firmware.elf
LD = xc32-gcc
$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	xc32-bin2hex $@
	$(Q)$(SIZE) $@

include $(TOP)/py/mkrules.mk
