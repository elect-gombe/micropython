include ../../py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE = /home/gombe/workspace/origdev/pic32/compiler/pic32-tools/bin/pic32-

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
CPU = 32MX370F512H

CFLAGS_CORTEX_M4 = -MMD -g -mprocessor=$(CPU) -c -fsingle-precision-constant -Wdouble-promotion -mips16 
CFLAGS = $(INC) -Wall -std=gnu99 $(CFLAGS_CORTEX_M4) $(COPT) $(CFLAGS_MOD) -g3  -fgnu89-inline
#DEBUG = 1
CFLAGS += -DMICROPY_FLOAT_IMPL=MICROPY_FLOAT_IMPL_FLOAT

#Debugging/Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0
else
CFLAGS += -O1 -DNDEBUG 
endif

LDFLAGS =  -mprocessor=$(CPU) -nostartfiles -Wl,--gc-sections,-Map=$@.map,--cref,--report-mem -T p32MX370F512H.ld
LIBS = -lm  /opt/microchip/xc32/v2.10/pic32mx/lib/libpic32.a

SRC_LIB = $(addprefix lib/,\
	libc/string0.c \
	oofatfs/ff.c \
	oofatfs/option/unicode.c \
	mp-readline/readline.c \
	netutils/netutils.c \
	timeutils/timeutils.c \
	utils/pyexec.c \
	utils/sys_stdio_mphal.c\
	utils/interrupt_char.c\
		)\
	$(addprefix lib/libm/,\
	math.c \
	acoshf.c \
	asinfacosf.c \
	asinhf.c \
	atan2f.c \
	atanf.c \
	atanhf.c \
	ef_rem_pio2.c \
	erf_lgamma.c \
	fmodf.c \
	kf_cos.c \
	kf_rem_pio2.c \
	kf_sin.c \
	kf_tan.c \
	log1pf.c \
	nearbyintf.c \
	sf_cos.c \
	sf_erf.c \
	sf_frexp.c \
	sf_ldexp.c \
	sf_modf.c \
	sf_sin.c \
	sf_tan.c \
	wf_lgamma.c \
	wf_tgamma.c \
	)\
	extmod/vfs_reader.c

SRC_C = \
	main.c \
	lib_video_megalopa.c\
	text_graph_library.c\
	flash.c\
	NVMem.c\
	modpyb.c\
	moduos.c\
	ps2keyboard.c
#	printf.c \
	string0.c \
	malloc0.c \
	gccollect.c \

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(SRC_MOD) $(SRC_LIB) $(EXTMOD_SRC_C)

SRC_S = \
	gc.s 

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_LIB:.c=.o) $(SRC_C:.c=.o) $(SRC_S:.s=.o)) \
crt0_mips32r2.o\
crti_mips32r2.o\
crtn_mips32r2.o

OBJ += $(addprefix $(BUILD)/, $(EXTMOD_SRC_C:.c=.o))


all: $(BUILD)/firmware.elf
LD = $(CC)



$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(CROSS_COMPILE)gcc $(LDFLAGS) -o $@ $^ $(LIBS)
	xc32-objdump build/firmware.elf -S > build/a.disa
	xc32-bin2hex $@
	$(Q)$(SIZE) $@

include $(TOP)/py/mkrules.mk
