LOCALAPPDATA := $(shell powershell -NoProfile -Command "[Environment]::GetFolderPath('LocalApplicationData')")
DIALUP_ROOT       := $(LOCALAPPDATA)/DialUp
include $(DIALUP_ROOT)/build-tools/common.mk
include $(DIALUP_ROOT)/build-tools/shell.mk
include $(DIALUP_ROOT)/build-tools/rocketleague.mk

BUILD_DIR      := build
EXE            := PatchUtilsTests.exe

.PHONY: configure build clean run

configure: check-shell
	@echo "🛠️ Configuring CMake..."
	$(call run_with_vcvars, cmake -S . -B build -G $(GENERATOR) -DCMAKE_BUILD_TYPE=RelWithDebInfo $(ARGS))

build: check-shell
	$(call run_with_vcvars, cmake --build build --config RelWithDebInfo $(ARGS))

clean: check-shell
	@rm -rf build

run: check-shell
	"$(BUILD_DIR)/$(EXE)"
