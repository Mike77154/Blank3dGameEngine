CC ?= gcc
CXX ?= g++
GCC ?= gcc
CLANG ?= clang
CLANGXX ?= clang++
AFL_CC ?= afl-clang-fast
AR ?= ar
RANLIB ?= ranlib
CMAKE ?= cmake
CPACK ?= cpack
PYTHON ?= python3
CORPUS_DIR ?= tests/fuzz_corpus
MIN_CORPUS_DIR ?= tests/fuzz_corpus_min
MIN_CORPUS_DIR_AFL ?= tests/fuzz_corpus_min_afl
MIN_CORPUS_DIR_AFL_TMIN ?= tests/fuzz_corpus_min_afl_tmin
MIN_CORPUS_DIR_INSPECT ?= tests/fuzz_corpus_min_inspect
MIN_CORPUS_DIR_RGB ?= tests/fuzz_corpus_min_rgb
MIN_CORPUS_DIR_INDEXED ?= tests/fuzz_corpus_min_indexed
CRASH_DIR ?= tests/fuzz_crashes
TRIAGE_DIR_LIBFUZZER ?= tests/fuzz_triage_libfuzzer
TRIAGE_DIR_STDIN ?= tests/fuzz_triage_stdin
TRIAGE_TIMEOUT ?= 10
FUZZ_RUNS ?= 256
CI_ARTIFACTS_DIR ?= tests/ci_artifacts
CI_LOG_DIR ?= tests/ci_logs
TEST_CFLAGS ?=
TEST_LDFLAGS ?=
TEST_SANITIZERS ?=
BUNDLE_SCRIPT ?= ./scripts/bundle_ci_artifacts.sh
RENDER_SCRIPT ?= ./scripts/render_ci_bundle.py
INSTALL ?= install
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig
DESTDIR ?=
VERSION := $(shell cat VERSION)
SONAME_MAJOR := 1
SHARED_BASENAME := libpcx.so
SHARED_REAL := $(SHARED_BASENAME).$(VERSION)
SHARED_SONAME := $(SHARED_BASENAME).$(SONAME_MAJOR)
UNAME_S := $(shell uname -s)
PCX_PC := pcx.pc
PCX_PUBLIC_HEADERS := include/pcx/pcx.h include/pcx/pcx_export.h include/pcx/pcx_config89.h
PCX_PRIVATE_HEADERS := pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h pcx_report.h
EXPORT_MAP := exports/pcx.map
EXPORT_ALLOWLIST := exports/pcx.exports.txt
HARDEN_CFLAGS ?= -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-strong
HARDEN_LDFLAGS ?= -Wl,-z,relro,-z,now
SHARED_VISIBILITY_CFLAGS ?= -fvisibility=hidden
CMAKE_BUILD_DIR ?= build-cmake
CMAKE_STAGE_DIR ?= $(CURDIR)/.cmake-stage
CMAKE_CONSUMER_BUILD_DIR ?= examples/cmake_consumer/build
CPACK_BUILD_DIR ?= build-cpack
PKGCONFIG_STAGE_DIR ?= $(CURDIR)/.pkgconfig-stage
PKGCONFIG_CONSUMER_BUILD_DIR ?= examples/pkgconfig_consumer/build
PACKAGE_REPORT_DIR ?= tests/package_reports
GENERATED_CORPUS_DIR ?= tests/generated_corpus
COMPAT_REPORT_DIR ?= tests/compat_reports
PERF_REPORT_DIR ?= tests/perf_reports
COVERAGE_REPORT_DIR ?= tests/coverage_reports
POLICY_REPORT_DIR ?= tests/policy_reports
GOVERNANCE_REPORT_DIR ?= tests/governance_reports
PORTABILITY_REPORT_DIR ?= tests/portability_reports
API_STAGE_DIR ?= $(CURDIR)/.api-stage
API_REPORT_DIR ?= tests/api_reports
ANALYZER_REPORT_DIR ?= tests/analyzer_reports
HEADER_BUILD_DIR ?= tests/headers_build
COMPILER_CONTRACT_REPORT_DIR ?= tests/compiler_contract_reports
DIST_DIR ?= dist
RELEASE_STAGE_DIR ?= .release/stage
RELEASE_PLATFORM_TAG ?= $(shell uname -s | tr A-Z a-z)-$(shell uname -m)
PROJECT_NAME := pcx
OPENSSL ?= openssl
SIGNING_KEY ?=
SIGNING_PUBLIC_KEY ?=
BUILDER_ID ?= urn:codec-release-tooling:local
BUILD_TYPE ?= https://example.com/codec/release/v1
SOURCE_URI ?=
INVOCATION_ID ?=
OWNER ?=
REPO ?=


CC_IS_CLANG := $(shell printf '' | $(CC) -dM -E - 2>/dev/null | grep -c __clang__)
CC_IS_GNU := $(shell printf '' | $(CC) -dM -E - 2>/dev/null | grep -c __GNUC__)
BASE_WARNING_CFLAGS ?= -Wall -Wextra -Werror -pedantic
COMPILER_WARNING_CFLAGS ?=
ifeq ($(CC_IS_CLANG),1)
COMPILER_WARNING_CFLAGS += -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wformat=2 -Wundef -Warray-bounds -Wnull-dereference
else ifeq ($(CC_IS_GNU),1)
COMPILER_WARNING_CFLAGS += -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wformat=2 -Wundef -Warray-bounds -Wnull-dereference -Walloc-zero -Wmismatched-dealloc -Wstringop-overread -Wstringop-overflow=2 -Warray-bounds=2 -Wrestrict
endif
CFLAGS ?= -std=c89 $(BASE_WARNING_CFLAGS) $(COMPILER_WARNING_CFLAGS)
CXXFLAGS ?= -Wall -Wextra -Werror -pedantic
CLANG_C89_CFLAGS ?= -std=c89 $(BASE_WARNING_CFLAGS) -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wformat=2 -Wundef -Warray-bounds -Wnull-dereference
LDFLAGS ?=
FUZZ_CFLAGS ?= -O1 -g -fno-omit-frame-pointer
FUZZ_SANITIZERS ?=

ifeq ($(strip $(FUZZ_SANITIZERS)),)
FUZZ_SANITIZE_FLAG := -fsanitize=fuzzer
else
FUZZ_SANITIZE_FLAG := -fsanitize=fuzzer,$(FUZZ_SANITIZERS)
endif

ifeq ($(strip $(TEST_SANITIZERS)),)
TEST_SANITIZE_FLAG :=
else
TEST_SANITIZE_FLAG := -fsanitize=$(TEST_SANITIZERS)
endif

ifeq ($(UNAME_S),Linux)
SHARED_VERSION_LDFLAGS := -Wl,-soname,$(SHARED_SONAME) -Wl,--version-script=$(EXPORT_MAP)
else
SHARED_VERSION_LDFLAGS :=
endif

SRCS = pcx_chunk.c pcx_parser.c pcx_render.c pcx_decoder.c pcx_encoder.c pcx_report.c
OBJS = $(SRCS:.c=.o)

.PHONY: ruleset-render governance-audit governance-plan governance-apply all static shared clean test test-smoke test-sanitize test-round7 test-round9 test-round10 ecosystem-audit pkgconfig-smoke cpack-smoke release-check diag-cli benchmark install uninstall pkgconfig corpus-generate fetch-external-corpus compat-report benchmark-report coverage-report observability-report release-package verify-release verify-repro verify-provenance fuzz-libfuzzer fuzz-stdin fuzz-afl fuzz-persistent fuzz-afl-persistent fuzz-libfuzzer-modes fuzz-stdin-modes seedcheck-libfuzzer seedcheck-stdin seedcheck-libfuzzer-modes seedcheck-stdin-modes seedcheck-libfuzzer-inspect seedcheck-libfuzzer-rgb seedcheck-libfuzzer-indexed seedcheck-stdin-inspect seedcheck-stdin-rgb seedcheck-stdin-indexed persistent-check corpus-min-libfuzzer corpus-min-afl corpus-tmin-afl corpus-min-libfuzzer-inspect corpus-min-libfuzzer-rgb corpus-min-libfuzzer-indexed crash-triage-libfuzzer crash-triage-stdin crash-buckets-libfuzzer crash-buckets-stdin repro-libfuzzer repro-stdin bundle-ci-inspect bundle-ci-rgb bundle-ci-indexed ci-fuzz-smoke abi-check test-asan test-ubsan test-sanitizers test-hardened cmake-smoke policy-audit preset-smoke portability-audit install-api-smoke api-surface-audit visibility-audit contract-audit contract-smoke warning-policy-audit header-matrix-smoke analyzer-smoke compiler-contracts-audit buffer-contract-smoke buffer-contracts-audit test-round15 test-round16

all: static shared $(PCX_PC)

static: libpcx.a

shared: $(SHARED_REAL) $(SHARED_SONAME) libpcx.so

libpcx.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

$(SHARED_REAL): $(SRCS) $(PCX_PRIVATE_HEADERS) $(EXPORT_MAP)
	$(CC) $(CFLAGS) $(SHARED_VISIBILITY_CFLAGS) -DPCX_BUILDING_LIBRARY -shared -fPIC -o $@ $(SRCS) $(LDFLAGS) $(SHARED_VERSION_LDFLAGS)

$(SHARED_SONAME): $(SHARED_REAL)
	ln -sf $(SHARED_REAL) $@

libpcx.so: $(SHARED_SONAME)
	ln -sf $(SHARED_SONAME) $@

$(PCX_PC): pcx.pc.in VERSION
	sed -e "s#@PREFIX@#$(PREFIX)#g" 	    -e "s#@LIBDIR@#$(LIBDIR)#g" 	    -e "s#@INCLUDEDIR@#$(INCLUDEDIR)#g" 	    -e "s#@VERSION@#$(VERSION)#g" $< > $@

pkgconfig: $(PCX_PC)

install: static shared $(PCX_PC)
	rm -f pcx.pc
	$(MAKE) pcx.pc PREFIX="$(PREFIX)" LIBDIR="$(LIBDIR)" INCLUDEDIR="$(INCLUDEDIR)"
	$(INSTALL) -d $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCLUDEDIR)/pcx $(DESTDIR)$(PKGCONFIGDIR)
	$(INSTALL) -m 644 libpcx.a $(DESTDIR)$(LIBDIR)/libpcx.a
	$(INSTALL) -m 755 $(SHARED_REAL) $(DESTDIR)$(LIBDIR)/$(SHARED_REAL)
	ln -sf $(SHARED_REAL) $(DESTDIR)$(LIBDIR)/$(SHARED_SONAME)
	ln -sf $(SHARED_SONAME) $(DESTDIR)$(LIBDIR)/libpcx.so
	for h in $(PCX_PUBLIC_HEADERS); do $(INSTALL) -m 644 $$h $(DESTDIR)$(INCLUDEDIR)/pcx/$$(basename $$h); done
	$(INSTALL) -m 644 $(PCX_PC) $(DESTDIR)$(PKGCONFIGDIR)/$(PCX_PC)

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/libpcx.a $(DESTDIR)$(LIBDIR)/libpcx.so $(DESTDIR)$(LIBDIR)/$(SHARED_SONAME) $(DESTDIR)$(LIBDIR)/$(SHARED_REAL) $(DESTDIR)$(PKGCONFIGDIR)/$(PCX_PC)
	for h in $(PCX_PUBLIC_HEADERS); do rm -f $(DESTDIR)$(INCLUDEDIR)/pcx/$$(basename $$h); done
	rmdir $(DESTDIR)$(INCLUDEDIR)/pcx 2>/dev/null || true

%.o: %.c pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h pcx_report.h
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -fPIC -c $< -o $@

tests/test_smoke: tests/test_smoke.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h pcx_report.h
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE $(TEST_CFLAGS) $(TEST_SANITIZE_FLAG) -I. -o $@ tests/test_smoke.c $(SRCS) $(LDFLAGS) $(TEST_LDFLAGS)

tools/pcx_diag_cli: tools/pcx_diag_cli.c $(SRCS) pcx.h pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h pcx_report.h
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -o $@ tools/pcx_diag_cli.c $(SRCS) $(LDFLAGS)

tools/pcx_compat_cli: tools/pcx_compat_cli.c $(SRCS) pcx.h pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h pcx_report.h
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -o $@ tools/pcx_compat_cli.c $(SRCS) $(LDFLAGS)

diag-cli: tools/pcx_diag_cli

tools/pcx_bench: tools/pcx_bench.c $(SRCS) pcx.h pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h pcx_report.h
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -o $@ tools/pcx_bench.c $(SRCS) $(LDFLAGS)

benchmark: tools/pcx_bench
	./tools/pcx_bench

corpus-generate:
	$(PYTHON) ./scripts/generate_rare_cases.py

fetch-external-corpus:
	$(PYTHON) ./scripts/fetch_external_corpus.py

compat-report: tools/pcx_compat_cli corpus-generate
	$(PYTHON) ./scripts/run_compat_matrix.py --cli tools/pcx_compat_cli --json-out $(COMPAT_REPORT_DIR)/compat_matrix.json --md-out $(COMPAT_REPORT_DIR)/compat_matrix.md

benchmark-report: tools/pcx_bench
	$(PYTHON) ./scripts/run_benchmarks.py --bench tools/pcx_bench --loops 256 --json-out $(PERF_REPORT_DIR)/benchmark.json --md-out $(PERF_REPORT_DIR)/benchmark.md

coverage-report:
	$(PYTHON) ./scripts/run_coverage.py --json-out $(COVERAGE_REPORT_DIR)/coverage.json --md-out $(COVERAGE_REPORT_DIR)/coverage.md

observability-report: compat-report benchmark-report coverage-report

policy-audit:
	$(PYTHON) ./scripts/audit_github_policy.py --repo-root . --approved-actions ./policies/approved_actions.json --json-out $(POLICY_REPORT_DIR)/github_policy_audit.json --md-out $(POLICY_REPORT_DIR)/github_policy_audit.md

ruleset-render:
	$(PYTHON) ./scripts/render_rulesets.py --required-checks ./governance/required_status_checks.json --output-dir ./.github/rulesets

governance-audit: ruleset-render
	$(PYTHON) ./scripts/audit_repo_governance.py --repo-root . --required-checks ./governance/required_status_checks.json --json-out $(GOVERNANCE_REPORT_DIR)/repository_governance_audit.json --md-out $(GOVERNANCE_REPORT_DIR)/repository_governance_audit.md

governance-plan: ruleset-render
	test -n "$(OWNER)" && test -n "$(REPO)"
	$(PYTHON) ./scripts/gh_apply_governance.py --repo-root . --owner "$(OWNER)" --repo "$(REPO)" --dry-run --out $(GOVERNANCE_REPORT_DIR)/github_apply_plan.sh

governance-apply: ruleset-render
	test -n "$(OWNER)" && test -n "$(REPO)"
	$(PYTHON) ./scripts/gh_apply_governance.py --repo-root . --owner "$(OWNER)" --repo "$(REPO)"

release-package:
	$(MAKE) clean
	$(PYTHON) ./scripts/build_release_artifacts.py --project $(PROJECT_NAME) --version $(VERSION) --repo-root . --dist-dir $(DIST_DIR) --platform-tag $(RELEASE_PLATFORM_TAG) --make "$(MAKE)" --openssl "$(OPENSSL)" $(if $(SIGNING_KEY),--signing-key "$(SIGNING_KEY)",) $(if $(SIGNING_PUBLIC_KEY),--signing-public-key "$(SIGNING_PUBLIC_KEY)",) --builder-id "$(BUILDER_ID)" --build-type "$(BUILD_TYPE)" $(if $(SOURCE_URI),--source-uri "$(SOURCE_URI)",) $(if $(INVOCATION_ID),--invocation-id "$(INVOCATION_ID)",)

verify-release:
	$(PYTHON) ./scripts/verify_release_artifacts.py --dist-dir $(DIST_DIR)

verify-repro:
	$(PYTHON) ./scripts/check_reproducible_release.py --project $(PROJECT_NAME) --version $(VERSION) --repo-root . --platform-tag $(RELEASE_PLATFORM_TAG) --make "$(MAKE)"

verify-provenance:
	$(PYTHON) ./scripts/verify_provenance.py --dist-dir $(DIST_DIR) --policy $(DIST_DIR)/verification-policy.json --expected-builder-id "$(BUILDER_ID)" --expected-build-type "$(BUILD_TYPE)" --expected-project $(PROJECT_NAME) --expected-version $(VERSION) --expected-platform $(RELEASE_PLATFORM_TAG) --openssl "$(OPENSSL)"


abi-check: $(SHARED_REAL) $(EXPORT_ALLOWLIST) scripts/check_exports.py
	$(PYTHON) ./scripts/check_exports.py --library ./$(SHARED_REAL) --allowlist $(EXPORT_ALLOWLIST) --expected-soname $(SHARED_SONAME)

test-asan:
	$(MAKE) clean
	$(MAKE) test-smoke TEST_CFLAGS="-O1 -g -fno-omit-frame-pointer" TEST_SANITIZERS=address
	./tests/test_smoke

test-ubsan:
	$(MAKE) clean
	$(MAKE) test-smoke TEST_CFLAGS="-O1 -g -fno-omit-frame-pointer" TEST_SANITIZERS=undefined
	./tests/test_smoke

test-sanitizers: test-asan test-ubsan

test-hardened:
	$(MAKE) clean
	$(MAKE) test CFLAGS="$(CFLAGS) $(HARDEN_CFLAGS)" LDFLAGS="$(LDFLAGS) $(HARDEN_LDFLAGS)"

cmake-smoke:
	rm -rf $(CMAKE_BUILD_DIR) $(CMAKE_STAGE_DIR) $(CMAKE_CONSUMER_BUILD_DIR) $(CPACK_BUILD_DIR) $(PKGCONFIG_STAGE_DIR) $(PKGCONFIG_CONSUMER_BUILD_DIR) $(PACKAGE_REPORT_DIR) $(PORTABILITY_REPORT_DIR) build/presets
	$(CMAKE) -S . -B $(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DPCX_BUILD_TESTS=OFF -DPCX_BUILD_TOOLS=OFF -DCMAKE_INSTALL_PREFIX=$(CMAKE_STAGE_DIR)
	$(CMAKE) --build $(CMAKE_BUILD_DIR)
	$(CMAKE) --install $(CMAKE_BUILD_DIR)
	$(CMAKE) -S examples/cmake_consumer -B $(CMAKE_CONSUMER_BUILD_DIR) -DCMAKE_PREFIX_PATH=$(CMAKE_STAGE_DIR)
	$(CMAKE) --build $(CMAKE_CONSUMER_BUILD_DIR)
	LD_LIBRARY_PATH=$(CMAKE_STAGE_DIR)/lib:$$LD_LIBRARY_PATH ./$(CMAKE_CONSUMER_BUILD_DIR)/pcx_consumer

pkgconfig-smoke:
	rm -rf $(PKGCONFIG_STAGE_DIR) $(PKGCONFIG_CONSUMER_BUILD_DIR)
	$(MAKE) clean
	$(MAKE) install DESTDIR="$(PKGCONFIG_STAGE_DIR)" PREFIX=/usr
	mkdir -p $(PKGCONFIG_CONSUMER_BUILD_DIR)
	PKG_CONFIG_SYSROOT_DIR="$(PKGCONFIG_STAGE_DIR)" PKG_CONFIG_PATH="$(PKGCONFIG_STAGE_DIR)/usr/lib/pkgconfig" $(CC) $(CFLAGS) examples/pkgconfig_consumer/main.c -o $(PKGCONFIG_CONSUMER_BUILD_DIR)/pcx_pkgconfig_consumer $$(PKG_CONFIG_SYSROOT_DIR="$(PKGCONFIG_STAGE_DIR)" PKG_CONFIG_PATH="$(PKGCONFIG_STAGE_DIR)/usr/lib/pkgconfig" pkg-config --cflags --libs pcx)
	LD_LIBRARY_PATH="$(PKGCONFIG_STAGE_DIR)/usr/lib:$$LD_LIBRARY_PATH" ./$(PKGCONFIG_CONSUMER_BUILD_DIR)/pcx_pkgconfig_consumer

preset-smoke:
	rm -rf build/presets
	$(CMAKE) --preset linux-gcc-release
	$(CMAKE) --build --preset linux-gcc-release
	ctest --preset linux-gcc-release
	$(CMAKE) --preset linux-clang-release
	$(CMAKE) --build --preset linux-clang-release
	ctest --preset linux-clang-release

portability-audit:
	$(PYTHON) ./scripts/audit_portability.py --repo-root . --json-out $(PORTABILITY_REPORT_DIR)/portability_audit.json --md-out $(PORTABILITY_REPORT_DIR)/portability_audit.md

install-api-smoke:
	rm -rf $(API_STAGE_DIR)
	$(MAKE) clean
	$(MAKE) install DESTDIR="$(API_STAGE_DIR)" PREFIX=/usr
	$(CC) $(CFLAGS) -I$(API_STAGE_DIR)/usr/include tests/test_public_api.c -L$(API_STAGE_DIR)/usr/lib -lpcx -o tests/test_public_api
	LD_LIBRARY_PATH="$(API_STAGE_DIR)/usr/lib:$$LD_LIBRARY_PATH" ./tests/test_public_api

api-surface-audit: install-api-smoke
	$(PYTHON) ./scripts/audit_api_surface.py --repo-root . --project pcx --stage-prefix "$(API_STAGE_DIR)/usr" --json-out $(API_REPORT_DIR)/api_surface_audit.json --md-out $(API_REPORT_DIR)/api_surface_audit.md

visibility-audit:
	$(PYTHON) ./scripts/audit_visibility.py --repo-root . --project pcx --json-out $(API_REPORT_DIR)/visibility_audit.json --md-out $(API_REPORT_DIR)/visibility_audit.md

contract-audit:
	mkdir -p $(API_REPORT_DIR)
	$(PYTHON) ./scripts/audit_contract_annotations.py --repo-root . --json-out $(API_REPORT_DIR)/contract_annotation_audit.json --md-out $(API_REPORT_DIR)/contract_annotation_audit.md

contract-smoke:
	mkdir -p tests/contracts_build
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -c tests/contracts/returns_nonnull_ok.c -o tests/contracts_build/returns_nonnull_ok.o
	@if $(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -c tests/contracts/ignore_result_fail.c -o tests/contracts_build/ignore_result_fail.o >/dev/null 2>&1; then echo "expected warn_unused_result failure"; exit 1; fi
	@if $(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -c tests/contracts/nonnull_fail.c -o tests/contracts_build/nonnull_fail.o >/dev/null 2>&1; then echo "expected nonnull failure"; exit 1; fi
	@if [ "$(CC_IS_CLANG)" = "0" ] && [ "$(CC_IS_GNU)" = "1" ]; then if $(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -c tests/contracts/mismatched_deallocator_fail.c -o tests/contracts_build/mismatched_deallocator_fail.o >/dev/null 2>&1; then echo "expected mismatched deallocator failure"; exit 1; fi; else echo "skipping GCC-only mismatched deallocator smoke for $(CC)"; fi

warning-policy-audit:
	mkdir -p $(API_REPORT_DIR)
	$(PYTHON) ./scripts/audit_warning_policy.py --repo-root . --project pcx --json-out $(API_REPORT_DIR)/warning_policy_audit.json --md-out $(API_REPORT_DIR)/warning_policy_audit.md
header-matrix-smoke:
	mkdir -p $(HEADER_BUILD_DIR)
	$(CC) -std=c89 $(BASE_WARNING_CFLAGS) $(COMPILER_WARNING_CFLAGS) -DPCX_STATIC_DEFINE -I./include -c tests/headers/pcx_c89_include.c -o $(HEADER_BUILD_DIR)/pcx_c89_include.o
	$(CC) -std=c99 $(BASE_WARNING_CFLAGS) $(COMPILER_WARNING_CFLAGS) -DPCX_STATIC_DEFINE -I./include -c tests/headers/pcx_c99_include.c -o $(HEADER_BUILD_DIR)/pcx_c99_include.o
	$(CC) -std=c11 $(BASE_WARNING_CFLAGS) $(COMPILER_WARNING_CFLAGS) -DPCX_STATIC_DEFINE -I./include -c tests/headers/pcx_c11_include.c -o $(HEADER_BUILD_DIR)/pcx_c11_include.o
	$(CXX) -std=c++11 $(CXXFLAGS) -DPCX_STATIC_DEFINE -I./include -c tests/headers/pcx_cpp11_include.cpp -o $(HEADER_BUILD_DIR)/pcx_cpp11_include.o

analyzer-smoke:
	mkdir -p $(ANALYZER_REPORT_DIR)
	@if command -v $(GCC) >/dev/null 2>&1; then \
		$(GCC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -fanalyzer -c pcx_chunk.c -o $(ANALYZER_REPORT_DIR)/pcx_chunk.gcc-analyzer.o; \
		$(GCC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -fanalyzer -c pcx_report.c -o $(ANALYZER_REPORT_DIR)/pcx_report.gcc-analyzer.o; \
	else \
		echo "skipping GCC analyzer smoke: $(GCC) not found"; \
	fi
	@if command -v $(CLANG) >/dev/null 2>&1; then \
		$(CLANG) -std=c89 $(BASE_WARNING_CFLAGS) -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wformat=2 -Wundef -Warray-bounds -Wnull-dereference -DPCX_STATIC_DEFINE -I. --analyze -Xanalyzer -analyzer-output=text pcx_chunk.c >$(ANALYZER_REPORT_DIR)/pcx_chunk.clang-analyzer.txt; \
		$(CLANG) -std=c89 $(BASE_WARNING_CFLAGS) -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wformat=2 -Wundef -Warray-bounds -Wnull-dereference -DPCX_STATIC_DEFINE -I. --analyze -Xanalyzer -analyzer-output=text pcx_report.c >$(ANALYZER_REPORT_DIR)/pcx_report.clang-analyzer.txt; \
	else \
		echo "skipping Clang analyzer smoke: $(CLANG) not found"; \
	fi

compiler-contracts-audit:
	mkdir -p $(COMPILER_CONTRACT_REPORT_DIR)
	$(PYTHON) ./scripts/audit_compiler_contracts.py --repo-root . --json-out $(COMPILER_CONTRACT_REPORT_DIR)/compiler_contract_audit.json --md-out $(COMPILER_CONTRACT_REPORT_DIR)/compiler_contract_audit.md

buffer-contract-smoke:
	mkdir -p tests/contracts_build
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -c tests/contracts/buffer_contracts_ok.c -o tests/contracts_build/buffer_contracts_ok.o
	@if [ "$(CC_IS_GNU)" = "1" ] && [ "$(CC_IS_CLANG)" = "0" ]; then \
		if $(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -c tests/contracts/access_read_overflow_fail.c -o tests/contracts_build/access_read_overflow_fail.o >/dev/null 2>&1; then echo "expected GCC access(read_only) failure"; exit 1; fi; \
		if $(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -c tests/contracts/access_write_overflow_fail.c -o tests/contracts_build/access_write_overflow_fail.o >/dev/null 2>&1; then echo "expected GCC access(write_only) failure"; exit 1; fi; \
	else \
		echo "skipping GCC-only buffer contract smoke for $(CC)"; \
	fi

buffer-contracts-audit:
	mkdir -p $(COMPILER_CONTRACT_REPORT_DIR)
	$(PYTHON) ./scripts/audit_buffer_contracts.py --repo-root . --json-out $(COMPILER_CONTRACT_REPORT_DIR)/buffer_contract_audit.json --md-out $(COMPILER_CONTRACT_REPORT_DIR)/buffer_contract_audit.md

test-round15: test-smoke contract-smoke warning-policy-audit header-matrix-smoke analyzer-smoke compiler-contracts-audit

test-round16: test-round15 buffer-contract-smoke buffer-contracts-audit


ecosystem-audit:
	$(PYTHON) ./scripts/audit_packaging.py --repo-root . --project $(PROJECT_NAME) --version $(VERSION) --json-out $(PACKAGE_REPORT_DIR)/packaging_audit.json --md-out $(PACKAGE_REPORT_DIR)/packaging_audit.md

cpack-smoke:
	rm -rf $(CPACK_BUILD_DIR)
	$(CMAKE) -S . -B $(CPACK_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DPCX_BUILD_TESTS=OFF -DPCX_BUILD_TOOLS=OFF
	$(CMAKE) --build $(CPACK_BUILD_DIR)
	cd $(CPACK_BUILD_DIR) && $(CPACK) -G TGZ && $(CPACK) -G ZIP && $(CPACK) -G TGZ --config CPackSourceConfig.cmake
	$(PYTHON) ./scripts/audit_packaging.py --repo-root . --project $(PROJECT_NAME) --version $(VERSION) --build-dir $(CPACK_BUILD_DIR) --json-out $(PACKAGE_REPORT_DIR)/cpack_audit.json --md-out $(PACKAGE_REPORT_DIR)/cpack_audit.md

tests/fuzz_pcx_libfuzzer: tests/fuzz_libfuzzer.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h
	$(CLANG) $(CLANG_C89_CFLAGS) -DPCX_STATIC_DEFINE $(FUZZ_CFLAGS) $(FUZZ_SANITIZE_FLAG) -I. -o $@ tests/fuzz_libfuzzer.c $(SRCS) $(LDFLAGS)

tests/fuzz_pcx_stdin: tests/fuzz_afl_stdin.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -o $@ tests/fuzz_afl_stdin.c $(SRCS) $(LDFLAGS)

tests/fuzz_pcx_afl: tests/fuzz_afl_stdin.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h
	$(AFL_CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -o $@ tests/fuzz_afl_stdin.c $(SRCS) $(LDFLAGS)

tests/fuzz_pcx_persistent: tests/fuzz_afl_persistent.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -o $@ tests/fuzz_afl_persistent.c $(SRCS) $(LDFLAGS)

tests/fuzz_pcx_afl_persistent: tests/fuzz_afl_persistent.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h
	$(AFL_CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -o $@ tests/fuzz_afl_persistent.c $(SRCS) $(LDFLAGS)

tests/fuzz_pcx_inspect_libfuzzer: tests/fuzz_libfuzzer_inspect.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h
	$(CLANG) $(CLANG_C89_CFLAGS) -DPCX_STATIC_DEFINE $(FUZZ_CFLAGS) $(FUZZ_SANITIZE_FLAG) -I. -o $@ tests/fuzz_libfuzzer_inspect.c $(SRCS) $(LDFLAGS)

tests/fuzz_pcx_rgb_libfuzzer: tests/fuzz_libfuzzer_rgb.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h
	$(CLANG) $(CLANG_C89_CFLAGS) -DPCX_STATIC_DEFINE $(FUZZ_CFLAGS) $(FUZZ_SANITIZE_FLAG) -I. -o $@ tests/fuzz_libfuzzer_rgb.c $(SRCS) $(LDFLAGS)

tests/fuzz_pcx_indexed_libfuzzer: tests/fuzz_libfuzzer_indexed.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h
	$(CLANG) $(CLANG_C89_CFLAGS) -DPCX_STATIC_DEFINE $(FUZZ_CFLAGS) $(FUZZ_SANITIZE_FLAG) -I. -o $@ tests/fuzz_libfuzzer_indexed.c $(SRCS) $(LDFLAGS)

tests/fuzz_pcx_inspect_stdin: tests/fuzz_stdin_inspect.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -o $@ tests/fuzz_stdin_inspect.c $(SRCS) $(LDFLAGS)

tests/fuzz_pcx_rgb_stdin: tests/fuzz_stdin_rgb.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -o $@ tests/fuzz_stdin_rgb.c $(SRCS) $(LDFLAGS)

tests/fuzz_pcx_indexed_stdin: tests/fuzz_stdin_indexed.c $(SRCS) pcx_chunk.h pcx_parser.h pcx_render.h pcx_decoder.h pcx_encoder.h
	$(CC) $(CFLAGS) -DPCX_STATIC_DEFINE -I. -o $@ tests/fuzz_stdin_indexed.c $(SRCS) $(LDFLAGS)

fuzz-libfuzzer: tests/fuzz_pcx_libfuzzer

fuzz-stdin: tests/fuzz_pcx_stdin

fuzz-afl: tests/fuzz_pcx_afl

fuzz-persistent: tests/fuzz_pcx_persistent

fuzz-afl-persistent: tests/fuzz_pcx_afl_persistent

fuzz-libfuzzer-modes: tests/fuzz_pcx_inspect_libfuzzer tests/fuzz_pcx_rgb_libfuzzer tests/fuzz_pcx_indexed_libfuzzer

fuzz-stdin-modes: tests/fuzz_pcx_inspect_stdin tests/fuzz_pcx_rgb_stdin tests/fuzz_pcx_indexed_stdin

test-smoke: tests/test_smoke
	./tests/test_smoke

test-sanitize: test-smoke

seedcheck-libfuzzer: tests/fuzz_pcx_libfuzzer
	./tests/fuzz_pcx_libfuzzer -runs=$(FUZZ_RUNS) $(CORPUS_DIR)

seedcheck-stdin: tests/fuzz_pcx_stdin
	for f in $(CORPUS_DIR)/*; do ./tests/fuzz_pcx_stdin $$f >/dev/null || exit 1; done

seedcheck-libfuzzer-inspect: tests/fuzz_pcx_inspect_libfuzzer
	./tests/fuzz_pcx_inspect_libfuzzer -runs=$(FUZZ_RUNS) $(CORPUS_DIR)

seedcheck-libfuzzer-rgb: tests/fuzz_pcx_rgb_libfuzzer
	./tests/fuzz_pcx_rgb_libfuzzer -runs=$(FUZZ_RUNS) $(CORPUS_DIR)

seedcheck-libfuzzer-indexed: tests/fuzz_pcx_indexed_libfuzzer
	./tests/fuzz_pcx_indexed_libfuzzer -runs=$(FUZZ_RUNS) $(CORPUS_DIR)

seedcheck-stdin-inspect: tests/fuzz_pcx_inspect_stdin
	for f in $(CORPUS_DIR)/*; do ./tests/fuzz_pcx_inspect_stdin $$f >/dev/null || exit 1; done

seedcheck-stdin-rgb: tests/fuzz_pcx_rgb_stdin
	for f in $(CORPUS_DIR)/*; do ./tests/fuzz_pcx_rgb_stdin $$f >/dev/null || exit 1; done

seedcheck-stdin-indexed: tests/fuzz_pcx_indexed_stdin
	for f in $(CORPUS_DIR)/*; do ./tests/fuzz_pcx_indexed_stdin $$f >/dev/null || exit 1; done

seedcheck-libfuzzer-modes: seedcheck-libfuzzer-inspect seedcheck-libfuzzer-rgb seedcheck-libfuzzer-indexed

seedcheck-stdin-modes: seedcheck-stdin-inspect seedcheck-stdin-rgb seedcheck-stdin-indexed

persistent-check: tests/fuzz_pcx_persistent
	for f in $(CORPUS_DIR)/*; do ./tests/fuzz_pcx_persistent $$f >/dev/null || exit 1; done

corpus-min-libfuzzer: tests/fuzz_pcx_libfuzzer
	sh ./scripts/minimize_corpus_libfuzzer.sh ./tests/fuzz_pcx_libfuzzer "$(CORPUS_DIR)" "$(MIN_CORPUS_DIR)"

corpus-min-libfuzzer-inspect: tests/fuzz_pcx_inspect_libfuzzer
	sh ./scripts/minimize_corpus_libfuzzer.sh ./tests/fuzz_pcx_inspect_libfuzzer "$(CORPUS_DIR)" "$(MIN_CORPUS_DIR_INSPECT)"

corpus-min-libfuzzer-rgb: tests/fuzz_pcx_rgb_libfuzzer
	sh ./scripts/minimize_corpus_libfuzzer.sh ./tests/fuzz_pcx_rgb_libfuzzer "$(CORPUS_DIR)" "$(MIN_CORPUS_DIR_RGB)"

corpus-min-libfuzzer-indexed: tests/fuzz_pcx_indexed_libfuzzer
	sh ./scripts/minimize_corpus_libfuzzer.sh ./tests/fuzz_pcx_indexed_libfuzzer "$(CORPUS_DIR)" "$(MIN_CORPUS_DIR_INDEXED)"

corpus-min-afl: tests/fuzz_pcx_stdin
	sh ./scripts/minimize_corpus_afl.sh ./tests/fuzz_pcx_stdin "$(CORPUS_DIR)" "$(MIN_CORPUS_DIR_AFL)"

corpus-tmin-afl: tests/fuzz_pcx_stdin
	sh ./scripts/tmin_corpus_afl.sh ./tests/fuzz_pcx_stdin "$(MIN_CORPUS_DIR_AFL)" "$(MIN_CORPUS_DIR_AFL_TMIN)"

crash-triage-libfuzzer: tests/fuzz_pcx_libfuzzer
	$(PYTHON) ./scripts/triage_crashes.py --mode libfuzzer --harness ./tests/fuzz_pcx_libfuzzer --input-dir "$(CRASH_DIR)" --output-dir "$(TRIAGE_DIR_LIBFUZZER)" --timeout "$(TRIAGE_TIMEOUT)" --minimize

crash-triage-stdin: tests/fuzz_pcx_stdin
	$(PYTHON) ./scripts/triage_crashes.py --mode file --harness ./tests/fuzz_pcx_stdin --input-dir "$(CRASH_DIR)" --output-dir "$(TRIAGE_DIR_STDIN)" --timeout "$(TRIAGE_TIMEOUT)"

crash-buckets-libfuzzer: crash-triage-libfuzzer

crash-buckets-stdin: crash-triage-stdin

repro-libfuzzer: tests/fuzz_pcx_libfuzzer
	sh ./scripts/repro_crash.sh libfuzzer ./tests/fuzz_pcx_libfuzzer "$(FILE)"

repro-stdin: tests/fuzz_pcx_stdin
	sh ./scripts/repro_crash.sh file ./tests/fuzz_pcx_stdin "$(FILE)"

bundle-ci-inspect:
	PYTHON=$(PYTHON) RENDER_SCRIPT=$(RENDER_SCRIPT) sh $(BUNDLE_SCRIPT) "$(CI_ARTIFACTS_DIR)/inspect" pcx inspect ./tests/fuzz_pcx_inspect_libfuzzer "$(CORPUS_DIR)" "$(MIN_CORPUS_DIR_INSPECT)" "$(CI_LOG_DIR)/inspect-libfuzzer.log" "$(CI_LOG_DIR)/inspect-stdin.log" "$(TRIAGE_DIR_LIBFUZZER)" "$(TRIAGE_DIR_STDIN)"

bundle-ci-rgb:
	PYTHON=$(PYTHON) RENDER_SCRIPT=$(RENDER_SCRIPT) sh $(BUNDLE_SCRIPT) "$(CI_ARTIFACTS_DIR)/rgb" pcx rgb ./tests/fuzz_pcx_rgb_libfuzzer "$(CORPUS_DIR)" "$(MIN_CORPUS_DIR_RGB)" "$(CI_LOG_DIR)/rgb-libfuzzer.log" "$(CI_LOG_DIR)/rgb-stdin.log" "$(TRIAGE_DIR_LIBFUZZER)" "$(TRIAGE_DIR_STDIN)"

bundle-ci-indexed:
	PYTHON=$(PYTHON) RENDER_SCRIPT=$(RENDER_SCRIPT) sh $(BUNDLE_SCRIPT) "$(CI_ARTIFACTS_DIR)/indexed" pcx indexed ./tests/fuzz_pcx_indexed_libfuzzer "$(CORPUS_DIR)" "$(MIN_CORPUS_DIR_INDEXED)" "$(CI_LOG_DIR)/indexed-libfuzzer.log" "$(CI_LOG_DIR)/indexed-stdin.log" "$(TRIAGE_DIR_LIBFUZZER)" "$(TRIAGE_DIR_STDIN)"

ci-fuzz-smoke: test test-smoke seedcheck-libfuzzer seedcheck-stdin persistent-check seedcheck-libfuzzer-modes seedcheck-stdin-modes

test-round7:
	$(PYTHON) ./tests/test_round7_tools.py

test-round9: test test-smoke

test-round10: test test-smoke tools/pcx_diag_cli
	./tools/pcx_diag_cli --json tests/fuzz_corpus/valid_indexed8_palette.pcx >/dev/null
	./tools/pcx_diag_cli --text tests/fuzz_corpus/valid_rgb24_round3.pcx >/dev/null

test-round11: test-round10 preset-smoke portability-audit install-api-smoke api-surface-audit visibility-audit contract-audit

test-round14: test-round11 contract-smoke warning-policy-audit contract-smoke warning-policy-audit

release-check: test-round11 seedcheck-stdin-modes

clean:
	rm -f $(OBJS) libpcx.a libpcx.so $(SHARED_SONAME) $(SHARED_REAL) tests/test_smoke tools/pcx_diag_cli tools/pcx_compat_cli tools/pcx_bench
	rm -f tests/fuzz_pcx_libfuzzer tests/fuzz_pcx_stdin tests/fuzz_pcx_afl tests/fuzz_pcx_persistent tests/fuzz_pcx_afl_persistent
	rm -f tests/fuzz_pcx_inspect_libfuzzer tests/fuzz_pcx_rgb_libfuzzer tests/fuzz_pcx_indexed_libfuzzer
	rm -f tests/fuzz_pcx_inspect_stdin tests/fuzz_pcx_rgb_stdin tests/fuzz_pcx_indexed_stdin
	rm -rf tests/.tmp_fixtures $(TRIAGE_DIR_LIBFUZZER) $(TRIAGE_DIR_STDIN) $(MIN_CORPUS_DIR) $(MIN_CORPUS_DIR_AFL) $(MIN_CORPUS_DIR_AFL_TMIN)
	rm -rf $(MIN_CORPUS_DIR_INSPECT) $(MIN_CORPUS_DIR_RGB) $(MIN_CORPUS_DIR_INDEXED) $(CI_ARTIFACTS_DIR) $(CI_LOG_DIR)
	rm -rf $(DIST_DIR) $(RELEASE_STAGE_DIR) .release $(CPACK_BUILD_DIR) $(PKGCONFIG_STAGE_DIR) $(PKGCONFIG_CONSUMER_BUILD_DIR) $(PACKAGE_REPORT_DIR) $(PORTABILITY_REPORT_DIR) $(API_STAGE_DIR) $(API_REPORT_DIR) $(ANALYZER_REPORT_DIR) $(HEADER_BUILD_DIR) $(COMPILER_CONTRACT_REPORT_DIR) build/presets
	rm -f tests/test_public_api

test: shared
	$(PYTHON) tests/test_decoder.py


test-round12: test-round11 visibility-audit
