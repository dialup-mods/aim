LOCALAPPDATA := $(shell powershell -NoProfile -Command "[Environment]::GetFolderPath('LocalApplicationData')")
DIALUP_ROOT       := $(LOCALAPPDATA)/DialUp
include $(DIALUP_ROOT)/build-tools/common.mk
include $(DIALUP_ROOT)/build-tools/shell.mk
include $(DIALUP_ROOT)/build-tools/rocketleague.mk

INJECTOR       := $(DIALUP_ROOT)/bin/DialUpInjector.exe
BUILD_DIR      := build
TEST_BIN       := build
SDK_DLL        := DialUp-SDK.dll
SDK_PDB        := DialUp-SDK.pdb
DLL            := AllTests.dll
PDB            := AllTests.pdb
DLL_RAND_NAME  := $(shell shuf -i 8-16 -n 1 | xargs openssl rand -hex)
SDK_RAND_NAME  := $(shell shuf -i 8-16 -n 1 | xargs openssl rand -hex)

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
	@bash -lc '\
		if [ -f dll_filename ]; then \
			rm "$(binary_dir)/$$(cat dll_filename).dll"; \
			rm "$(binary_dir)/$$(cat dll_filename).pdb"; \
		fi ; \
		touch dll_filename ; \
		echo $(DLL_RAND_NAME) > dll_filename; \
		cp -v "$(BUILD_DIR)/$(DLL)" "$(binary_dir)/$(DLL_RAND_NAME).dll"; \
		cp -v "$(BUILD_DIR)/$(PDB)" "$(binary_dir)/$(DLL_RAND_NAME).pdb"; \
		if [ -f dll_filename_sdk ]; then \
			rm "$(binary_dir)/$$(cat dll_filename_sdk).dll"; \
			rm "$(binary_dir)/$$(cat dll_filename_sdk).pdb"; \
		fi ; \
		touch dll_filename_sdk ; \
		echo $(SDK_RAND_NAME) > dll_filename_sdk; \
		cp -v "$(DIALUP_ROOT)/sdk-plugin/bin/$(SDK_DLL)" "$(binary_dir)/$(SDK_RAND_NAME).dll"; \
		cp -v "$(DIALUP_ROOT)/sdk-plugin/bin/$(SDK_PDB)" "$(binary_dir)/$(SDK_RAND_NAME).pdb"; \
		'
	"$(INJECTOR)" "$(target)" "$(binary_dir)/$(DLL_RAND_NAME).dll"
	"$(INJECTOR)" "$(target)" "$(binary_dir)/$(SDK_RAND_NAME).dll"
	"$(INJECTOR)" "$(target)" "$(DLL)"
