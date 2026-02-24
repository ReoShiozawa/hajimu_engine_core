# =============================================================================
# engine_core — はじむ言語用プラグイン
# クロスプラットフォーム Makefile (macOS / Linux / Windows MinGW)
# =============================================================================

PLUGIN_NAME = engine_core
BUILD_DIR   = build
OUTPUT      = $(BUILD_DIR)/$(PLUGIN_NAME).hjp

# OS 判定 ($(OS) は Windows CMD/PowerShell で "Windows_NT" になる)
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
    INSTALL_DIR := $(USERPROFILE)/.hajimu/plugins
    NCPU        := $(NUMBER_OF_PROCESSORS)
else
    DETECTED_OS := $(shell uname -s 2>/dev/null || echo Unknown)
    INSTALL_DIR := $(HOME)/.hajimu/plugins
    NCPU        := $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
endif

CMAKE_FLAGS = -DCMAKE_BUILD_TYPE=Release -Wno-dev -DHAJIMU_INCLUDE_DIR=$(HAJIMU_INCLUDE)
# はじむインクルードパス自動検出
ifeq ($(OS),Windows_NT)
    ifndef HAJIMU_INCLUDE
        HAJIMU_INCLUDE := $(or \
            $(if $(wildcard ../../jp/include/hajimu.h),../../jp/include),\
            $(if $(wildcard ../jp/include/hajimu.h),../jp/include),\
            ./include)
    endif
else
    ifndef HAJIMU_INCLUDE
        HAJIMU_INCLUDE := $(shell \
            if [ -d "../../jp/include" ]; then echo "../../jp/include"; \
            elif [ -d "../jp/include" ]; then echo "../jp/include"; \
            elif [ -d "/usr/local/include/hajimu" ]; then echo "/usr/local/include/hajimu"; \
            else echo "include"; fi)
    endif
endif

.PHONY: all clean install uninstall

all: $(OUTPUT)

$(OUTPUT): CMakeLists.txt
	cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)
	cmake --build $(BUILD_DIR) -j$(NCPU)
	@echo "  ビルド完了: $(OUTPUT)"
clean:
ifeq ($(OS),Windows_NT)
	-rmdir /S /Q $(BUILD_DIR) 2>NUL
else
	rm -rf $(BUILD_DIR)
endif
	@echo "  クリーン完了"
install: all
ifeq ($(OS),Windows_NT)
	if not exist "$(INSTALL_DIR)\$(PLUGIN_NAME)" mkdir "$(INSTALL_DIR)\$(PLUGIN_NAME)"
	copy /Y hajimu.json "$(INSTALL_DIR)\$(PLUGIN_NAME)"
	copy /Y $(OUTPUT) "$(INSTALL_DIR)\$(PLUGIN_NAME)"
else
	@mkdir -p $(INSTALL_DIR)/$(PLUGIN_NAME)
	cp hajimu.json $(INSTALL_DIR)/$(PLUGIN_NAME)/
	cp $(OUTPUT) $(INSTALL_DIR)/$(PLUGIN_NAME)/
endif
	@echo "  インストール完了: $(INSTALL_DIR)/$(PLUGIN_NAME)/"

uninstall:
ifeq ($(OS),Windows_NT)
	-rmdir /S /Q "$(INSTALL_DIR)\$(PLUGIN_NAME)" 2>NUL
else
	rm -rf $(INSTALL_DIR)/$(PLUGIN_NAME)
endif
	@echo "  アンインストール完了"
