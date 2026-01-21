LOCALAPPDATA   := $(shell powershell -NoProfile -Command "[Environment]::GetFolderPath('LocalApplicationData')")
ROAMINGAPPDATA := $(shell powershell -NoProfile -Command "[Environment]::GetFolderPath('ApplicationData')")

DIALUP_ROOT       := $(LOCALAPPDATA)/DialUp
include $(DIALUP_ROOT)/build-tools/common.mk
include $(DIALUP_ROOT)/build-tools/shell.mk
include $(DIALUP_ROOT)/build-tools/rocketleague.mk

INSTALL_DIR    := $(DIALUP_ROOT)/plugin/AIM/bin
DLL := AIM.dll
PDB := AIM.pdb

.PHONY: configure build install clean

configure: check-shell
	$(call run_with_vcvars, cmake -S . -B build -G $(GENERATOR) -DCMAKE_BUILD_TYPE=RelWithDebInfo)

build: check-shell
	$(call run_with_vcvars, cmake --build build --config RelWithDebInfo)

install: check-shell
	$(call run_with_vcvars, cmake --install build --config RelWithDebInfo)
	@bash -lc '\
		cp -v "$(INSTALL_DIR)/$(DLL)" "$(ROAMINGAPPDATA)/bakkesmod/plugins/TestPlugin.dll"; \
		cp -v "$(INSTALL_DIR)/$(PDB)" "$(ROAMINGAPPDATA)/bakkesmod/plugins/TestPlugin.pdb"; \
		'

clean: check-shell
	@rm -rf build
