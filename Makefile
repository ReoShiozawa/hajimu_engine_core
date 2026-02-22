# ============================================================================
# engine_core — はじむ用ゲームエンジンコア
# Makefile (CMake ラッパー + hajimu プラグイン向け便利ターゲット)
# ============================================================================

PLUGIN_NAME = engine_core

# はじむインクルードパス
HAJIMU_INCLUDE ?= $(shell \
	if [ -d "../../jp/include" ]; then echo "../../jp/include"; \
	elif [ -d "../jp/include" ]; then echo "../jp/include"; \
	elif [ -d "/usr/local/include/hajimu" ]; then echo "/usr/local/include/hajimu"; \
	else echo "include"; fi)

# インストール先
INSTALL_DIR = $(HOME)/.hajimu/plugins/$(PLUGIN_NAME)

BUILD_DIR = build
CMAKE_FLAGS = -DCMAKE_BUILD_TYPE=Release -DHAJIMU_INCLUDE_DIR=$(HAJIMU_INCLUDE)

# ── ビルド ───────────────────────────────────────────────
.PHONY: all clean install test

all:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. $(CMAKE_FLAGS) -G "Unix Makefiles" 2>/dev/null
	@cd $(BUILD_DIR) && $(MAKE) --no-print-directory
	@cp $(BUILD_DIR)/$(PLUGIN_NAME).hjp .
	@echo ""
	@echo "  ✅ ビルド成功: $(PLUGIN_NAME).hjp"
	@echo ""
	@echo "  インストール:   make install"
	@echo "  テスト:         make test"
	@echo ""

clean:
	@rm -rf $(BUILD_DIR) $(PLUGIN_NAME).hjp
	@echo "  🧹 クリーン完了"

install: all
	@mkdir -p $(INSTALL_DIR)
	@cp $(PLUGIN_NAME).hjp $(INSTALL_DIR)/
	@cp hajimu.json $(INSTALL_DIR)/
	@echo ""
	@echo "  📦 インストール完了: $(INSTALL_DIR)/"
	@echo ""

test:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. $(CMAKE_FLAGS) -DBUILD_TESTS=ON -G "Unix Makefiles" 2>/dev/null
	@cd $(BUILD_DIR) && $(MAKE) --no-print-directory
	@cd $(BUILD_DIR) && ctest --output-on-failure
