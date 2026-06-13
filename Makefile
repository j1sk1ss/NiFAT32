CC ?= cc
AR ?= ar
RM ?= rm -f
MKDIR_P ?= mkdir -p

BUILD_DIR ?= builds
OBJ_DIR := $(BUILD_DIR)/obj

SHARED_LIBRARY := $(BUILD_DIR)/nifat32.so
STATIC_LIBRARY := $(BUILD_DIR)/nifat32.a
UNIX_EXECUTABLE := $(BUILD_DIR)/unix_nifat32
CONFIG_FILE := $(OBJ_DIR)/.build-flags

LIB_SOURCES := nifat32.c $(wildcard src/*.c) $(wildcard std/*.c)
LIB_OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(LIB_SOURCES))
UNIX_OBJECT := $(OBJ_DIR)/unix_nifat32.o
DEPENDENCIES := $(LIB_OBJECTS:.o=.d) $(UNIX_OBJECT:.o=.d)

CPPFLAGS ?=
CFLAGS ?= -O2
LDFLAGS ?=
LDLIBS ?=

NIFAT32_CPPFLAGS := -Iinclude
NIFAT32_CFLAGS := -std=gnu11 -Wall -Wextra -fPIC -MMD -MP

DEBUG ?= 0

# Logging groups
ERROR_LOGS ?= 0
WARN_LOGS ?= 0
INFO_LOGS ?= 0
DEBUG_LOGS ?= 0
IO_LOGS ?= 0
MEM_LOGS ?= 0
LOGGING_LOGS ?= 0
SPECIAL_LOGS ?= 0

# Feature switches
NIFAT32_RO ?= 0
NO_HEAP ?= 0
NO_DEFAULT_MM_MANAGER ?= 0
NIFAT32_NO_ERROR ?= 0
NIFAT32_NO_ECACHE ?= 0
NO_FAT_CACHE ?= 0
NO_FAT_MAP ?= 0
NO_ENTRY_VALIDATION ?= 0

# Numeric configuration
ALLOC_BUFFER_SIZE ?=
CONTENT_TABLE_SIZE ?=
IO_THREADS_MAX ?=

ifeq ($(DEBUG),1)
  NIFAT32_CFLAGS += -O0 -g3
endif

ifeq ($(ERROR_LOGS),1)
  NIFAT32_CPPFLAGS += -DERROR_LOGS
endif
ifeq ($(WARN_LOGS),1)
  NIFAT32_CPPFLAGS += -DWARNING_LOGS
endif
ifeq ($(INFO_LOGS),1)
  NIFAT32_CPPFLAGS += -DINFO_LOGS
endif
ifeq ($(DEBUG_LOGS),1)
  NIFAT32_CPPFLAGS += -DDEBUG_LOGS
endif
ifeq ($(IO_LOGS),1)
  NIFAT32_CPPFLAGS += -DIO_OPERATION_LOGS
endif
ifeq ($(MEM_LOGS),1)
  NIFAT32_CPPFLAGS += -DMEM_OPERATION_LOGS
endif
ifeq ($(LOGGING_LOGS),1)
  NIFAT32_CPPFLAGS += -DLOGGING_LOGS
endif
ifeq ($(SPECIAL_LOGS),1)
  NIFAT32_CPPFLAGS += -DSPECIAL_LOGS
endif

ifeq ($(NIFAT32_RO),1)
  NIFAT32_CPPFLAGS += -DNIFAT32_RO
endif
ifeq ($(NO_HEAP),1)
  NIFAT32_CPPFLAGS += -DNO_HEAP
endif
ifeq ($(NO_DEFAULT_MM_MANAGER),1)
  NIFAT32_CPPFLAGS += -DNO_DEFAULT_MM_MANAGER
endif
ifeq ($(NIFAT32_NO_ERROR),1)
  NIFAT32_CPPFLAGS += -DNIFAT32_NO_ERROR
endif
ifeq ($(NIFAT32_NO_ECACHE),1)
  NIFAT32_CPPFLAGS += -DNIFAT32_NO_ECACHE
endif
ifeq ($(NO_FAT_CACHE),1)
  NIFAT32_CPPFLAGS += -DNO_FAT_CACHE
endif
ifeq ($(NO_FAT_MAP),1)
  NIFAT32_CPPFLAGS += -DNO_FAT_MAP
endif
ifeq ($(NO_ENTRY_VALIDATION),1)
  NIFAT32_CPPFLAGS += -DNO_ENTRY_VALIDATION
endif

ifneq ($(strip $(ALLOC_BUFFER_SIZE)),)
  NIFAT32_CPPFLAGS += -DALLOC_BUFFER_SIZE=$(ALLOC_BUFFER_SIZE)
endif
ifneq ($(strip $(CONTENT_TABLE_SIZE)),)
  NIFAT32_CPPFLAGS += -DCONTENT_TABLE_SIZE=$(CONTENT_TABLE_SIZE)
endif
ifneq ($(strip $(IO_THREADS_MAX)),)
  NIFAT32_CPPFLAGS += -DIO_THREADS_MAX=$(IO_THREADS_MAX)
endif

.DEFAULT_GOAL := all

.PHONY: all shared static unix formatter tools clean distclean help FORCE

all: shared

shared: $(SHARED_LIBRARY)

static: $(STATIC_LIBRARY)

unix: $(UNIX_EXECUTABLE)

formatter:
	$(MAKE) -C formatter

tools: formatter unix

$(SHARED_LIBRARY): $(LIB_OBJECTS) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) -shared -o $@ $(LIB_OBJECTS) $(LDLIBS)

$(STATIC_LIBRARY): $(LIB_OBJECTS) | $(BUILD_DIR)
	$(AR) rcs $@ $(LIB_OBJECTS)

$(UNIX_EXECUTABLE): $(LIB_OBJECTS) $(UNIX_OBJECT) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) -o $@ $(UNIX_OBJECT) $(LIB_OBJECTS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.c $(CONFIG_FILE)
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(NIFAT32_CPPFLAGS) $(CFLAGS) $(NIFAT32_CFLAGS) -c $< -o $@

$(CONFIG_FILE): FORCE | $(OBJ_DIR)
	@printf '%s\n' '$(CC) $(CPPFLAGS) $(NIFAT32_CPPFLAGS) $(CFLAGS) $(NIFAT32_CFLAGS)' > $@.tmp
	@cmp -s $@.tmp $@ 2>/dev/null || cp $@.tmp $@
	@$(RM) $@.tmp

$(BUILD_DIR) $(OBJ_DIR):
	$(MKDIR_P) $@

FORCE:

clean:
	$(RM) -r $(OBJ_DIR)
	$(RM) $(SHARED_LIBRARY) $(STATIC_LIBRARY) $(UNIX_EXECUTABLE)

distclean: clean
	$(MAKE) -C formatter clean

help:
	@printf '%s\n' \
	  'Targets:' \
	  '  all, shared  Build builds/nifat32.so (default)' \
	  '  static       Build builds/nifat32.a' \
	  '  unix         Build builds/unix_nifat32' \
	  '  formatter    Build formatter/formatter' \
	  '  tools        Build the formatter and Unix utility' \
	  '  clean        Remove root build artifacts' \
	  '  distclean    Also clean the formatter' \
	  '' \
	  'Examples:' \
	  '  make DEBUG=1 ERROR_LOGS=1 WARN_LOGS=1' \
	  '  make static NIFAT32_RO=1 NO_HEAP=1' \
	  '  make unix LOGGING_LOGS=1'

-include $(DEPENDENCIES)
