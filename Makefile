# Built binary name
APP_NAME := App_3d_render

ifndef SDK_DIR
$(error You need to define the SDK_DIR environment variable, and point it to the sdk/ folder)
endif

# Directory structure
BUILD_DIR := build
SRC_DIR := src
LINKER_DIR := linker
SUB_DIRS := $(shell find $(SRC_DIR) -type d)  # List of subdirectories in SRC_DIR

SUB_DIRS_FOLDER_ONLY := $(shell cd $(SRC_DIR) && find . -type d | sed 's,^[^/]*/,,'  | sed -e 's/^/$(BUILD_DIR)\//')  # List of subdirectories in SRC_DIR

APP_ELF := $(APP_NAME).hhk

# Global defines
FIXPOINT_DEFS := -DFIXMATH_NO_CACHE -DFIXMATH_NO_CTYPE -DFIXMATH_NO_HARD_DIVISION -DFIXMATH_FAST_SIN -DFIXMATH_NO_64BIT

# Toolchain
AS := sh4-elf-as
AS_FLAGS :=
CC := sh4-elf-gcc
CC_FLAGS := -ffreestanding -fshort-wchar -Wall -Wextra -O2 -I $(SDK_DIR)/include/ $(FIXPOINT_DEFS)
CXX := sh4-elf-g++
CXX_FLAGS := -ffreestanding -fno-exceptions -fno-rtti -fshort-wchar -Wall -Wextra -O2 -I $(SDK_DIR)/include/ -m4a-nofpu $(FIXPOINT_DEFS)
LD := sh4-elf-ld
LD_FLAGS := -nostdlib --no-undefined
READELF := sh4-elf-readelf
OBJCOPY := sh4-elf-objcopy

# Source files
AS_SOURCES := $(wildcard $(SRC_DIR)/*.s) $(foreach dir,$(SUB_DIRS),$(wildcard $(dir)/*.s))
CC_SOURCES := $(wildcard $(SRC_DIR)/*.c) $(foreach dir,$(SUB_DIRS),$(wildcard $(dir)/*.c))
CXX_SOURCES := $(wildcard $(SRC_DIR)/*.cpp) $(foreach dir,$(SUB_DIRS),$(wildcard $(dir)/*.cpp))

# Object files
AS_OBJECTS := $(patsubst $(SRC_DIR)/%.s,$(BUILD_DIR)/%.o,$(AS_SOURCES))
CC_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(CC_SOURCES))
CXX_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CXX_SOURCES))

# Remove duplicates from object lists
AS_OBJECTS := $(sort $(AS_OBJECTS))
CC_OBJECTS := $(sort $(CC_OBJECTS))
CXX_OBJECTS := $(sort $(CXX_OBJECTS))

OBJECTS := $(AS_OBJECTS) $(CC_OBJECTS) $(CXX_OBJECTS)

# Targets
.PHONY: all hhk clean

all: $(APP_ELF)

hhk: $(APP_ELF)

clean:
	rm -f $(APP_ELF) $(OBJECTS)
	rm -rf $(BUILD_DIR)

PC: all
	./makepc
	./pc_out

$(APP_ELF): $(OBJECTS) $(SDK_DIR)/sdk.o $(LINKER_DIR)/linker_hhk.ld
	$(LD) -T $(LINKER_DIR)/linker_hhk.ld -o $@ $(LD_FLAGS) $(OBJECTS) $(SDK_DIR)/sdk.o
	$(OBJCOPY) --set-section-flags .hollyhock_name=contents,strings,readonly $(APP_ELF) $(APP_ELF)
	$(OBJCOPY) --set-section-flags .hollyhock_description=contents,strings,readonly $(APP_ELF) $(APP_ELF)
	$(OBJCOPY) --set-section-flags .hollyhock_author=contents,strings,readonly $(APP_ELF) $(APP_ELF)
	$(OBJCOPY) --set-section-flags .hollyhock_version=contents,strings,readonly $(APP_ELF) $(APP_ELF)

# Rule to compile assembly source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s | $(BUILD_DIR)
	$(AS) $< -o $@ $(AS_FLAGS)

# Rule to compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) -c $< -o $@ $(CC_FLAGS)

# Rule to compile C++ source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) -c $< -o $@ $(CXX_FLAGS)
	@$(READELF) $@ -S | grep ".ctors" > /dev/null && echo "ERROR: Global constructors aren't supported." && rm $@ && exit 1 || exit 0

# Rule to create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(SUB_DIRS_FOLDER_ONLY)


