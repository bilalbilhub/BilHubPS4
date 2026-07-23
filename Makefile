##############################################################
# BilHubPS4 - OpenOrbis Makefile
# المطلوب: تثبيت OpenOrbis Toolchain أولاً
# https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain
##############################################################

TOOLCHAIN ?= $(OO_PS4_TOOLCHAIN)

ifeq ($(TOOLCHAIN),)
$(error "Set OO_PS4_TOOLCHAIN environment variable first!\nexport OO_PS4_TOOLCHAIN=/path/to/toolchain")
endif

CC    = $(TOOLCHAIN)/bin/clang
CXX   = $(TOOLCHAIN)/bin/clang++
LD    = $(TOOLCHAIN)/bin/ld.lld
AR    = $(TOOLCHAIN)/bin/llvm-ar
MKSFO = $(TOOLCHAIN)/bin/mksfoex
FSELF = $(TOOLCHAIN)/bin/create-fself
PKGEN = $(TOOLCHAIN)/bin/pkg_gen

# ─── App Info ────────────────────────────────────────────────
TITLE_ID    = BILH00001
CONTENT_ID  = IV0000-BILH00001_00-BILHUBPS4000000
APP_VER     = 01.00
APP_TITLE   = BilHubPS4
PASSCODE    = 00000000000000000000000000000000

# ─── Directories ─────────────────────────────────────────────
SRC_DIR  = src
OBJ_DIR  = build/obj
OUT_DIR  = build

# ─── Sources ─────────────────────────────────────────────────
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# ─── Flags ───────────────────────────────────────────────────
CXXFLAGS = \
    --target=x86_64-scei-ps4 \
    -fPIC \
    -funwind-tables \
    -std=c++17 \
    -O2 \
    -I$(TOOLCHAIN)/include \
    -I$(TOOLCHAIN)/include/c++/v1 \
    -I$(SRC_DIR)

LDFLAGS = \
    -m elf_x86_64 \
    -pie \
    --script $(TOOLCHAIN)/link.x \
    --eh-frame-hdr \
    -L$(TOOLCHAIN)/lib \
    -lSceLibcInternal \
    -lSceSysmodule_stub_weak \
    -lScePad_stub_weak \
    -lSceVideoOut_stub_weak \
    -lSceNet_stub_weak \
    -lSceHttp_stub_weak \
    -lSceSsl_stub_weak \
    -lSceUserService_stub_weak \
    -lSceSystemService_stub_weak \
    -lkernel_stub_weak \
    -lc++ \
    -lc++abi

# ─── Output files ────────────────────────────────────────────
ELF  = $(OUT_DIR)/BilHubPS4.elf
OELF = $(OUT_DIR)/BilHubPS4.oelf
SFO  = $(OUT_DIR)/sce_sys/param.sfo
PKG  = $(OUT_DIR)/BilHubPS4.pkg

# ─── Targets ─────────────────────────────────────────────────
.PHONY: all clean pkg

all: pkg

# إنشاء مجلدات
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OUT_DIR)/sce_sys:
	mkdir -p $(OUT_DIR)/sce_sys

# ترجمة ملفات المصدر
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@echo "  CXX  $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ربط ELF
$(ELF): $(OBJS)
	@echo "  LD   $@"
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# OELF (PS4 signed ELF)
$(OELF): $(ELF)
	@echo "  FSELF  $@"
	$(FSELF) -in=$(ELF) -out=$(OELF) --paid 0x3800000000000011

# param.sfo
$(SFO): | $(OUT_DIR)/sce_sys
	@echo "  MKSFO  $@"
	$(MKSFO) \
		-d ATTRIBUTE=0 \
		-d CATEGORY=gd \
		-d APP_VER=$(APP_VER) \
		-d TITLE_ID=$(TITLE_ID) \
		-d CONTENT_ID=$(CONTENT_ID) \
		"$(APP_TITLE)" $(SFO)

# PKG النهائي
$(PKG): $(OELF) $(SFO)
	@echo "  PKG  $@"
	$(PKGEN) \
		--title-id $(TITLE_ID) \
		--passcode $(PASSCODE) \
		--content-id $(CONTENT_ID) \
		--pkg-output-path $(PKG) \
		--sfo $(SFO) \
		--main-module $(OELF) \
		--icon0 sce_sys/icon0.png \
		--extra-file downloads.json=/app/downloads.json

pkg: $(PKG)
	@echo ""
	@echo "  ✅  Build complete!"
	@echo "  📦  PKG: $(PKG)"
	@echo ""
	@echo "  Install on PS4 via Package Installer"

clean:
	@echo "  CLEAN"
	rm -rf build/
