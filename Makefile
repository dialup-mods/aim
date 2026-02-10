LOCALAPPDATA   := $(shell powershell -NoProfile -Command "[Environment]::GetFolderPath('LocalApplicationData')")
ROAMINGAPPDATA := $(shell powershell -NoProfile -Command "[Environment]::GetFolderPath('ApplicationData')")

DIALUP_ROOT       := $(LOCALAPPDATA)/DialUp
include $(DIALUP_ROOT)/build-tools/common.mk
include $(DIALUP_ROOT)/build-tools/shell.mk
include $(DIALUP_ROOT)/build-tools/rocketleague.mk

INSTALL_DIR    := $(DIALUP_ROOT)/plugin/AIM/bin
DLL := AIM.dll
PDB := AIM.pdb

.PHONY: configure build install clean all

configure: check-shell
	$(call run_with_vcvars, cmake -S . -B build -G $(GENERATOR) -DCMAKE_BUILD_TYPE=RelWithDebInfo)

build: check-shell
	$(call run_with_vcvars, cmake --build build --config RelWithDebInfo)

install: check-shell
	$(call run_with_vcvars, cmake --install build --config RelWithDebInfo)

clean: check-shell
	@rm -rf build

all: check-shell clean configure build install

msvc-spawn:
	@bash -c "if [ -f .build_status ]; do rm .build-status; done;"
build-aim-tests:
	@rm -f .build_status
	@PANE_ID=$$(wezterm cli spawn --new-window); \
	bash -c "while [ ! -f .build_status ]; do sleep 0.5; done; \
		if grep -q OK .build_status; then \
			echo '✓ Build succeeded'; \
			wezterm cli kill-pane --pane-id $$PANE_ID; \
		else \
			echo '✗ Build FAILED - focusing build window'; \
			wezterm cli activate-pane --pane-id $$PANE_ID; \
		fi"
#	@wezterm cli spawn --new-tab --cwd . --tab-title "MSVC Build" -- bash scripts/vs-build.sh 
#	@bash -lc '\
#		while [ ! -f .build_status ]; do sleep 0.5; done; \
#		if grep -q OK .build_status; then \
#			echo "✓ Build succeeded"; \
#		else \
#			echo "✗ Build FAILED - check build tab"; \
#			wezterm cli activate-tab --tab-title "MSVC Build"; \
#		fi'
