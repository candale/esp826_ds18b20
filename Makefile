include user.cfg

###############################################################################
# Toolchain Options
###############################################################################

#Note: user.cfg can override these.

GCC_FOLDER = $(ESP_ROOT)/xtensa-lx106-elf
ESPTOOL_PY = $(ESP_ROOT)/esptool/esptool.py
SDK       ?= $(ESP_ROOT)/sdk

XTLIB        = $(SDK)/lib
XTGCCLIB     = $(GCC_FOLDER)/lib/gcc/xtensa-lx106-elf/$(ESP_GCC_VERS)/libgcc.a
XCLIB        = $(GCC_FOLDER)/xtensa-lx106-elf/sysroot/lib/libc.a
FOLDERPREFIX = $(GCC_FOLDER)/bin
PREFIX       = $(FOLDERPREFIX)/xtensa-lx106-elf-
CC           = $(PREFIX)gcc
LD           = $(PREFIX)ld
AR           = $(PREFIX)ar
CP           = cp

SLOWTICK_MS  = 50

###############################################################################
# Derived Options
###############################################################################

SDK_DEFAULT = $(HOME)/esp8266/esp-open-sdk
ESP_ROOT := $(shell echo "$$ESP_ROOT")
ifeq ($(strip $(ESP_ROOT)),)
    $(warning Warning: No shell variable 'ESP_ROOT', using '$(SDK_DEFAULT)')
    ESP_ROOT := $(SDK_DEFAULT)
endif
VERSION := $(shell git describe --abbrev=5 --dirty=-dev --always --tags)
VERSSTR := "Version: $(VERSION) - Build $(shell date) with $(OPTS)"
PROJECT_NAME := $(notdir $(shell git rev-parse --show-toplevel))
PROJECT_URL := $(subst .com:,.com/,$(subst .git,,$(subst git@,https://,$(shell git config --get remote.origin.url))))



define USAGE
Usage: make [command] [VARIABLES]

all....... Build the binaries
debug..... Build a .map and .lst file.  Useful for seeing what's going on
           behind the scenes.
burn ..... Write firmware to chip using a regular serial port
getips ... Get a list with IPs of esp82xxs connected to your network
usbburn .. Burn via PRE-RELEASE USB loader (will probably change)
usbweb ... Burn webpage by PRE-RELEASE USB loader (will probably change)


More commands: all, clean, purge, dumprom
endef
.PHONY : all clean cleanall $(BIN_TARGET) getips help serterm
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))


# PROPRIETARY CODE
FW_FILE1 	= image.elf-0x00000.bin
FW_FILE2 	= image.elf-0x10000.bin
TARGET 		= image.elf

THIRD_PARTY_MODULES_DIR = lib

# Sources to be compiled
SRCS =  user/user_main.c \
		modules/wifi.c \

# Project includes
INCL = $(SDK)/include modules/include include


# THIRD PARTY CODE
# Paths to third party libs (normally in lib/) separated by space.
THIRD_PARTY_MODULES  = onewire ds18b20
SRCS 				 += $(foreach sdir,$(THIRD_PARTY_MODULES),$(wildcard $(sdir)/*.c))
THIRD_PARTY_INCLUDES =

# Try to get include directory from third party libs
MODULE_INCDIR 	:= $(addsuffix /include,$(THIRD_PARTY_MODULES))
INCL 			+= $(MODULE_INCDIR)
INCL 			+= $(THIRD_PARTY_INCLUDES)


# COMPILE FLAGS
CFLAGS = -mlongcalls -Os -ffunction-sections $(addprefix -I,$(INCL) $(call uniq, $(patsubst %/,%,$(dir $(SRCS))))) $(OPTS) -DVERSSTR='$(VERSSTR)'
LDFLAGS_CORE = -nostdlib -Wl,--start-group -lmain -lnet80211 -lcrypto -lssl -lwpa -llwip -lpp -lphy -Wl,--end-group -lgcc $(XCLIB) -T $(SDK)/ld/eagle.app.v6.ld

LINKFLAGS = $(LDFLAGS_CORE) -B$(XTLIB)

BIN_TARGET = $(PROJECT_NAME)-$(VERSION)-binaries.zip

ifneq (,$(findstring -DDEBUG,$(OPTS)))
$(warning Debug is enabled!)
FLASH_WRITE_FLAGS += --verify
endif

##########################################################################RULES

help :
	$(info $(value USAGE))
	@true

all : $(FW_FILE1) $(FW_FILE2)

$(FW_FILE1) $(FW_FILE2) : $(TARGET)
	PATH=$(PATH):$(FOLDERPREFIX) $(ESPTOOL_PY) elf2image $(TARGET)

$(TARGET) : $(SRCS) Makefile
	$(CC) $(CFLAGS) $(SRCS) -flto $(LINKFLAGS) -o $@

debug : $(TARGET)
	nm -S -n $(TARGET) > image.map
	$(PREFIX)objdump -S $(TARGET) > image.lst
	$(PREFIX)size -A $(TARGET) | grep -v debug


burn : $(FW_FILE1) $(FW_FILE2)
	($(ESPTOOL_PY) $(FWBURNFLAGS) --port $(PORT) write_flash --flash_mode dio $(FLASH_WRITE_FLAGS) 0x00000 $(FW_FILE1) 0x10000 $(FW_FILE2))||(true)

getips:
	$(info Detecting possible IPs for ESP82XX modules...)
	$(info Needs 'nmap' and takes some time especiallay if none are connected)
	sudo nmap -sP 192.168.0.0/24 | grep -iP "espressif|esp" -B2 | grep -oP "(\d{1,3}\.){3,3}\d\d{1,3}"

clean :
	$(RM) $(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SRCS))) $(TARGET) image.map image.lst $(CLEAN_EXTRA)

dumprom :
	($(ESPTOOL_PY) $(FWBURNFLAGS)  --port $(PORT) read_flash 0 1048576 dump.bin)||(true)

purge : clean
	@cd web && $(MAKE) $(MFLAGS) $(MAKEOVERRIDES) clean
	$(RM) $(FW_FILE1) $(FW_FILE2) $(BIN_TARGET)

serterm :
	screen $(PORT) $(BAUD)
