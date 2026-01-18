LOCALAPPDATA := $(shell powershell -NoProfile -Command "[Environment]::GetFolderPath('LocalApplicationData')")
DIALUP_ROOT       := $(LOCALAPPDATA)/DialUp
include $(DIALUP_ROOT)/build-tools/common.mk
include $(DIALUP_ROOT)/build-tools/shell.mk
include $(DIALUP_ROOT)/build-tools/rocketleague.mk

INJECTOR       := $(DIALUP_ROOT)/bin/DialUpInjector.exe
TEST_BIN       := build
SDK_DLL        := DialUp-SDK.dll
DLL            := AllTests.dll

.PHONY: configure build clean inject

configure: check-shell
	@echo "🛠️ Configuring CMake..."
	$(call run_with_vcvars, cmake -S . -B build -G $(GENERATOR) -DCMAKE_BUILD_TYPE=RelWithDebInfo $(ARGS))

build:
	$(call run_with_vcvars, cmake --build build --config RelWithDebInfo $(ARGS))

clean:
	@rm -rf build

#run-only: $(TEST_EXE)
#	@echo Copying DialUpFramework.dll next to test executable
#	cp "$(DIALUP_DLL)" "$(TEST_BIN)"
#	@echo Running tests
#	"$(TEST_EXE)"

run:
	# copy to RL bin dir, run, remove
	@bash -lc 'cp -v "$(TEST_BIN)/$(DLL)" "$(binary_dir)/$(DLL)"'
	"$(INJECTOR)" "$(target)" "$(SDK_DLL)"
	"$(INJECTOR)" "$(target)" "$(DLL)"