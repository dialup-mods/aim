PROBABLYMICROSOFT := 1
ifeq ($(strip $(MSYSTEM)),)
	PROBABLYMICROSOFT := 0
endif

game     ?= default
platform ?= epic
target   ?= RocketLeague.exe

# set binary_dir unless it was explicitly set via command line
ifeq ($(origin binary_dir), undefined)
ifeq ($(platform), steam)
    binary_dir := C:/Program Files (x86)/Steam/steamapps/common/rocketleague/Binaries/Win64
else ifeq ($(platform), epic)
	binary_dir := C:/Program Files/Epic Games/rocketleague/Binaries/Win64
else
    $(error Unknown platform: $(platform))
endif
endif

GENERATOR := Ninja
MAKEFLAGS += --no-print-directory
VCVARS := C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat
EXCEPTION_FILE := ../cmake/shell-exception.txt
INJECTOR       := $(LOCALAPPDATA)/DialUpFramework/bin/DialUpInjector.exe
TEST_BIN       := build
SDK_DLL        := DialUp-SDK.dll
DLL            := AllTests.dll

.PHONY: configure build install clean inject install-toolchain

define run_with_vcvars
	@powershell -NoLogo -NoProfile -Command \
		"cmd /C 'call \"$(VCVARS)\" >nul 2>&1 && $(1)'"
endef

define run_without_vcvars
	@powershell -NoLogo -NoProfile -Command \
		"$$env:DOCTEST_OPTIONS='--success --duration --no-capture'; \
		 cmd /C 'call \"$(VCVARS)\" >nul 2>&1 && $(1)'"
endef

check-shell:
	@if [ "$(PROBABLYMICROSOFT)" = "1" ]; then \
		cat $(EXCEPTION_FILE); \
		exit 1; \
	fi

configure: check-shell
	@echo "🛠️ Configuring CMake..."
	$(call run_with_vcvars, cmake -S . -B build -G $(GENERATOR) -DCMAKE_BUILD_TYPE=RelWithDebInfo $(ARGS))

build: configure
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