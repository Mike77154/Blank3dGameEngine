CC ?= cc
CXX ?= g++
GCC ?= gcc
CLANG ?= clang
CLANGXX ?= clang++
AFL_CC ?= afl-clang-fast
AR ?= ar
ARFLAGS ?= rcs
CMAKE ?= cmake
CPACK ?= cpack
PYTHON ?= python3
CORPUS_DIR ?= tests/fuzz_corpus
MIN_CORPUS_DIR ?= tests/fuzz_corpus_min
MIN_CORPUS_DIR_AFL ?= tests/fuzz_corpus_min_afl
MIN_CORPUS_DIR_AFL_TMIN ?= tests/fuzz_corpus_min_afl_tmin
MIN_CORPUS_DIR_PARSE ?= tests/fuzz_corpus_min_parse
MIN_CORPUS_DIR_DECODE ?= tests/fuzz_corpus_min_decode
MIN_CORPUS_DIR_PAYLOAD ?= tests/fuzz_corpus_min_payload
CRASH_DIR ?= tests/fuzz_crashes
TRIAGE_DIR_LIBFUZZER ?= tests/fuzz_triage_libfuzzer
TRIAGE_DIR_STDIN ?= tests/fuzz_triage_stdin
TRIAGE_TIMEOUT ?= 10
FUZZ_CFLAGS ?= -O1 -g -fno-omit-frame-pointer
FUZZ_SANITIZERS ?=
FUZZ_RUNS ?= 256
CI_ARTIFACTS_DIR ?= tests/ci_artifacts
CI_LOG_DIR ?= tests/ci_logs
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
SHARED_BASENAME := libbmp.so
SHARED_REAL := $(SHARED_BASENAME).$(VERSION)
SHARED_SONAME := $(SHARED_BASENAME).$(SONAME_MAJOR)
UNAME_S := $(shell uname -s)
BMP_PC := bmp.pc
BMP_PUBLIC_HEADERS := include/bmp/bmp.h include/bmp/bmp_export.h
BMP_PRIVATE_HEADERS := $(filter-out bmp/bmp.h,$(wildcard bmp/*.h))
EXPORT_MAP := exports/bmp.map
EXPORT_ALLOWLIST := exports/bmp.exports.txt
TEST_CFLAGS ?=
TEST_LDFLAGS ?=
TEST_SANITIZERS ?=
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
PROJECT_NAME := bmp
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
BASE_WARNING_CFLAGS ?= -Wall -Wextra -Wpedantic -Werror
COMPILER_WARNING_CFLAGS ?=
ifeq ($(CC_IS_CLANG),1)
COMPILER_WARNING_CFLAGS += -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wformat=2 -Wundef -Warray-bounds -Wnull-dereference
else ifeq ($(CC_IS_GNU),1)
COMPILER_WARNING_CFLAGS += -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wformat=2 -Wundef -Warray-bounds -Wnull-dereference -Wstringop-overread -Wstringop-overflow=2 -Warray-bounds=2 -Wrestrict
endif
CFLAGS ?= -std=c89 -pedantic-errors -O2 $(BASE_WARNING_CFLAGS) $(COMPILER_WARNING_CFLAGS)
CXXFLAGS ?= -Wall -Wextra -Werror -pedantic
LDFLAGS ?=

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

SRC := 	bmp/bmp_chunks.c 	bmp/bmp_metadata.c 	bmp/bmp_parser.c 	bmp/bmp_report.c 	bmp/bmp_decoder.c 	bmp/bmp_encoder.c 	bmp/bmp_filters.c 	bmp/bmp_render.c

OBJ := $(SRC:.c=.o)

.PHONY: protocol-audit fuzz-c89 fuzz-c89-modes preset-smoke portability-audit install-api-smoke api-surface-audit visibility-audit contract-audit contract-smoke warning-policy-audit header-matrix-smoke analyzer-smoke compiler-contracts-audit buffer-contract-smoke buffer-contracts-audit test-round15 test-round16 test-round11  ruleset-render governance-audit governance-plan governance-apply all shared pkgconfig install uninstall diag-cli benchmark abi-check test-asan test-ubsan test-sanitizers test-hardened cmake-smoke policy-audit test fuzz-libfuzzer fuzz-stdin fuzz-afl fuzz-persistent fuzz-afl-persistent fuzz-libfuzzer-modes fuzz-stdin-modes seedcheck-libfuzzer seedcheck-stdin seedcheck-libfuzzer-parse seedcheck-libfuzzer-decode seedcheck-libfuzzer-payload seedcheck-stdin-parse seedcheck-stdin-decode seedcheck-stdin-payload seedcheck-libfuzzer-modes seedcheck-stdin-modes persistent-check corpus-min-libfuzzer corpus-min-libfuzzer-parse corpus-min-libfuzzer-decode corpus-min-libfuzzer-payload corpus-min-afl corpus-tmin-afl crash-triage-libfuzzer crash-triage-stdin crash-buckets-libfuzzer crash-buckets-stdin repro-libfuzzer repro-stdin bundle-ci-parse bundle-ci-decode bundle-ci-payload ci-fuzz-smoke corpus-generate fetch-external-corpus compat-report benchmark-report coverage-report observability-report release-package verify-release verify-repro verify-provenance

all: libbmp.a shared $(BMP_PC)

shared: $(SHARED_REAL) $(SHARED_SONAME) libbmp.so

libbmp.a: $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

$(SHARED_REAL): $(OBJ) $(EXPORT_MAP)
	$(CC) $(CFLAGS) $(SHARED_VISIBILITY_CFLAGS) -DBMP_BUILDING_LIBRARY -shared -fPIC -o $@ $(SRC) $(LDFLAGS) $(SHARED_VERSION_LDFLAGS)

$(SHARED_SONAME): $(SHARED_REAL)
	ln -sf $(SHARED_REAL) $@

libbmp.so: $(SHARED_SONAME)
	ln -sf $(SHARED_SONAME) $@

$(BMP_PC): bmp.pc.in VERSION
	sed -e "s#@PREFIX@#$(PREFIX)#g" 	    -e "s#@LIBDIR@#$(LIBDIR)#g" 	    -e "s#@INCLUDEDIR@#$(INCLUDEDIR)#g" 	    -e "s#@VERSION@#$(VERSION)#g" $< > $@

pkgconfig: $(BMP_PC)

install: libbmp.a shared $(BMP_PC)
	rm -f bmp.pc
	$(MAKE) bmp.pc PREFIX="$(PREFIX)" LIBDIR="$(LIBDIR)" INCLUDEDIR="$(INCLUDEDIR)"
	$(INSTALL) -d $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCLUDEDIR)/bmp $(DESTDIR)$(PKGCONFIGDIR)
	$(INSTALL) -m 644 libbmp.a $(DESTDIR)$(LIBDIR)/libbmp.a
	$(INSTALL) -m 755 $(SHARED_REAL) $(DESTDIR)$(LIBDIR)/$(SHARED_REAL)
	ln -sf $(SHARED_REAL) $(DESTDIR)$(LIBDIR)/$(SHARED_SONAME)
	ln -sf $(SHARED_SONAME) $(DESTDIR)$(LIBDIR)/libbmp.so
	for h in $(BMP_PUBLIC_HEADERS); do $(INSTALL) -m 644 $$h $(DESTDIR)$(INCLUDEDIR)/bmp/$$(basename $$h); done
	$(INSTALL) -m 644 $(BMP_PC) $(DESTDIR)$(PKGCONFIGDIR)/$(BMP_PC)

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/libbmp.a $(DESTDIR)$(LIBDIR)/libbmp.so $(DESTDIR)$(LIBDIR)/$(SHARED_SONAME) $(DESTDIR)$(LIBDIR)/$(SHARED_REAL) $(DESTDIR)$(PKGCONFIGDIR)/$(BMP_PC)
	for h in $(BMP_PUBLIC_HEADERS); do rm -f $(DESTDIR)$(INCLUDEDIR)/bmp/$$(basename $$h); done
	rmdir $(DESTDIR)$(INCLUDEDIR)/bmp 2>/dev/null || true

%.o: %.c
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -fPIC -c -o $@ $<

tests/test_smoke: tests/test_smoke.c libbmp.a
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE $(TEST_CFLAGS) $(TEST_SANITIZE_FLAG) -I./bmp -o $@ $< ./libbmp.a $(TEST_LDFLAGS)

tools/bmp_diag_cli: tools/bmp_diag_cli.c $(SRC) bmp/bmp.h
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I./bmp -o $@ $< $(SRC)

tools/bmp_compat_cli: tools/bmp_compat_cli.c $(SRC) bmp/bmp.h
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I./bmp -o $@ $< $(SRC)

diag-cli: tools/bmp_diag_cli

tools/bmp_bench: tools/bmp_bench.c $(SRC) bmp/bmp.h
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I./bmp -o $@ $< $(SRC)

benchmark: tools/bmp_bench
	./tools/bmp_bench

corpus-generate:
	$(PYTHON) ./scripts/generate_rare_cases.py

fetch-external-corpus:
	$(PYTHON) ./scripts/fetch_external_corpus.py

compat-report: tools/bmp_compat_cli corpus-generate
	$(PYTHON) ./scripts/run_compat_matrix.py --cli tools/bmp_compat_cli --json-out $(COMPAT_REPORT_DIR)/compat_matrix.json --md-out $(COMPAT_REPORT_DIR)/compat_matrix.md

benchmark-report: tools/bmp_bench
	$(PYTHON) ./scripts/run_benchmarks.py --bench tools/bmp_bench --loops 256 --json-out $(PERF_REPORT_DIR)/benchmark.json --md-out $(PERF_REPORT_DIR)/benchmark.md

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
	$(MAKE) test TEST_CFLAGS="-O1 -g -fno-omit-frame-pointer" TEST_SANITIZERS=address

test-ubsan:
	$(MAKE) clean
	$(MAKE) test TEST_CFLAGS="-O1 -g -fno-omit-frame-pointer" TEST_SANITIZERS=undefined

test-sanitizers: test-asan test-ubsan

test-hardened:
	$(MAKE) clean
	$(MAKE) test CFLAGS="$(CFLAGS) $(HARDEN_CFLAGS)" LDFLAGS="$(LDFLAGS) $(HARDEN_LDFLAGS)"

cmake-smoke:
	rm -rf $(CMAKE_BUILD_DIR) $(CMAKE_STAGE_DIR) $(CMAKE_CONSUMER_BUILD_DIR) $(CPACK_BUILD_DIR) $(PKGCONFIG_STAGE_DIR) $(PKGCONFIG_CONSUMER_BUILD_DIR) $(PACKAGE_REPORT_DIR) $(PORTABILITY_REPORT_DIR) $(API_STAGE_DIR) $(API_REPORT_DIR) $(ANALYZER_REPORT_DIR) $(HEADER_BUILD_DIR) $(COMPILER_CONTRACT_REPORT_DIR) build/presets
	rm -f tests/test_public_api
	$(CMAKE) -S . -B $(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DBMP_BUILD_TESTS=OFF -DBMP_BUILD_TOOLS=OFF -DCMAKE_INSTALL_PREFIX=$(CMAKE_STAGE_DIR)
	$(CMAKE) --build $(CMAKE_BUILD_DIR)
	$(CMAKE) --install $(CMAKE_BUILD_DIR)
	$(CMAKE) -S examples/cmake_consumer -B $(CMAKE_CONSUMER_BUILD_DIR) -DCMAKE_PREFIX_PATH=$(CMAKE_STAGE_DIR)
	$(CMAKE) --build $(CMAKE_CONSUMER_BUILD_DIR)
	LD_LIBRARY_PATH=$(CMAKE_STAGE_DIR)/lib:$$LD_LIBRARY_PATH ./$(CMAKE_CONSUMER_BUILD_DIR)/bmp_consumer

pkgconfig-smoke:
	rm -rf $(PKGCONFIG_STAGE_DIR) $(PKGCONFIG_CONSUMER_BUILD_DIR)
	$(MAKE) clean
	$(MAKE) install DESTDIR="$(PKGCONFIG_STAGE_DIR)" PREFIX=/usr
	mkdir -p $(PKGCONFIG_CONSUMER_BUILD_DIR)
	PKG_CONFIG_SYSROOT_DIR="$(PKGCONFIG_STAGE_DIR)" PKG_CONFIG_PATH="$(PKGCONFIG_STAGE_DIR)/usr/lib/pkgconfig" $(CC) $(CFLAGS) examples/pkgconfig_consumer/main.c -o $(PKGCONFIG_CONSUMER_BUILD_DIR)/bmp_pkgconfig_consumer $$(PKG_CONFIG_SYSROOT_DIR="$(PKGCONFIG_STAGE_DIR)" PKG_CONFIG_PATH="$(PKGCONFIG_STAGE_DIR)/usr/lib/pkgconfig" pkg-config --cflags --libs bmp)
	LD_LIBRARY_PATH="$(PKGCONFIG_STAGE_DIR)/usr/lib:$$LD_LIBRARY_PATH" ./$(PKGCONFIG_CONSUMER_BUILD_DIR)/bmp_pkgconfig_consumer

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
	$(CC) $(CFLAGS) -I$(API_STAGE_DIR)/usr/include tests/test_public_api.c -L$(API_STAGE_DIR)/usr/lib -lbmp -o tests/test_public_api
	LD_LIBRARY_PATH="$(API_STAGE_DIR)/usr/lib:$$LD_LIBRARY_PATH" ./tests/test_public_api

api-surface-audit: install-api-smoke
	$(PYTHON) ./scripts/audit_api_surface.py --repo-root . --project bmp --stage-prefix "$(API_STAGE_DIR)/usr" --json-out $(API_REPORT_DIR)/api_surface_audit.json --md-out $(API_REPORT_DIR)/api_surface_audit.md

visibility-audit:
	$(PYTHON) ./scripts/audit_visibility.py --repo-root . --project bmp --json-out $(API_REPORT_DIR)/visibility_audit.json --md-out $(API_REPORT_DIR)/visibility_audit.md

contract-audit:
	mkdir -p $(API_REPORT_DIR)
	$(PYTHON) ./scripts/audit_contract_annotations.py --repo-root . --json-out $(API_REPORT_DIR)/contract_annotation_audit.json --md-out $(API_REPORT_DIR)/contract_annotation_audit.md

contract-smoke:
	mkdir -p tests/contracts_build
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I./bmp -c tests/contracts/returns_nonnull_ok.c -o tests/contracts_build/returns_nonnull_ok.o
	@if $(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I./bmp -c tests/contracts/ignore_result_fail.c -o tests/contracts_build/ignore_result_fail.o >/dev/null 2>&1; then echo "expected warn_unused_result failure"; exit 1; fi
	@if $(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I./bmp -c tests/contracts/nonnull_fail.c -o tests/contracts_build/nonnull_fail.o >/dev/null 2>&1; then echo "expected nonnull failure"; exit 1; fi

warning-policy-audit:
	mkdir -p $(API_REPORT_DIR)
	$(PYTHON) ./scripts/audit_warning_policy.py --repo-root . --project bmp --json-out $(API_REPORT_DIR)/warning_policy_audit.json --md-out $(API_REPORT_DIR)/warning_policy_audit.md
header-matrix-smoke:
	mkdir -p $(HEADER_BUILD_DIR)
	$(CC) -std=c89 -pedantic-errors $(BASE_WARNING_CFLAGS) $(COMPILER_WARNING_CFLAGS) -DBMP_STATIC_DEFINE -I./include -c tests/headers/bmp_c89_include.c -o $(HEADER_BUILD_DIR)/bmp_c89_include.o


analyzer-smoke:
	mkdir -p $(ANALYZER_REPORT_DIR)
	@if command -v $(GCC) >/dev/null 2>&1; then \
		$(GCC) $(CFLAGS) -DBMP_STATIC_DEFINE -I./bmp -fanalyzer -c bmp/bmp_parser.c -o $(ANALYZER_REPORT_DIR)/bmp_parser.gcc-analyzer.o; \
		$(GCC) $(CFLAGS) -DBMP_STATIC_DEFINE -I./bmp -fanalyzer -c bmp/bmp_report.c -o $(ANALYZER_REPORT_DIR)/bmp_report.gcc-analyzer.o; \
	else \
		echo "skipping GCC analyzer smoke: $(GCC) not found"; \
	fi
	@if command -v $(CLANG) >/dev/null 2>&1; then \
		$(CLANG) -std=c89 -pedantic-errors $(BASE_WARNING_CFLAGS) -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wformat=2 -Wundef -Warray-bounds -Wnull-dereference -DBMP_STATIC_DEFINE -I./bmp --analyze -Xanalyzer -analyzer-output=text bmp/bmp_parser.c >$(ANALYZER_REPORT_DIR)/bmp_parser.clang-analyzer.txt; \
		$(CLANG) -std=c89 -pedantic-errors $(BASE_WARNING_CFLAGS) -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith -Wformat=2 -Wundef -Warray-bounds -Wnull-dereference -DBMP_STATIC_DEFINE -I./bmp --analyze -Xanalyzer -analyzer-output=text bmp/bmp_report.c >$(ANALYZER_REPORT_DIR)/bmp_report.clang-analyzer.txt; \
	else \
		echo "skipping Clang analyzer smoke: $(CLANG) not found"; \
	fi

compiler-contracts-audit:
	mkdir -p $(COMPILER_CONTRACT_REPORT_DIR)
	$(PYTHON) ./scripts/audit_compiler_contracts.py --repo-root . --json-out $(COMPILER_CONTRACT_REPORT_DIR)/compiler_contract_audit.json --md-out $(COMPILER_CONTRACT_REPORT_DIR)/compiler_contract_audit.md

buffer-contract-smoke:
	mkdir -p tests/contracts_build
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I. -I./include -c tests/contracts/buffer_contracts_ok.c -o tests/contracts_build/buffer_contracts_ok.o
	@if [ "$(CC_IS_GNU)" = "1" ] && [ "$(CC_IS_CLANG)" = "0" ]; then \
		if $(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I. -I./include -c tests/contracts/access_read_overflow_fail.c -o tests/contracts_build/access_read_overflow_fail.o >/dev/null 2>&1; then echo "expected GCC access(read_only) failure"; exit 1; fi; \
		if $(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I. -I./include -c tests/contracts/access_write_overflow_fail.c -o tests/contracts_build/access_write_overflow_fail.o >/dev/null 2>&1; then echo "expected GCC access(write_only) failure"; exit 1; fi; \
	else \
		echo "skipping GCC-only buffer contract smoke for $(CC)"; \
	fi

buffer-contracts-audit:
	mkdir -p $(COMPILER_CONTRACT_REPORT_DIR)
	$(PYTHON) ./scripts/audit_buffer_contracts.py --repo-root . --json-out $(COMPILER_CONTRACT_REPORT_DIR)/buffer_contract_audit.json --md-out $(COMPILER_CONTRACT_REPORT_DIR)/buffer_contract_audit.md

test-round15: test contract-smoke warning-policy-audit header-matrix-smoke analyzer-smoke compiler-contracts-audit

test-round16: test-round15 buffer-contract-smoke buffer-contracts-audit


ecosystem-audit:
	$(PYTHON) ./scripts/audit_packaging.py --repo-root . --project $(PROJECT_NAME) --version $(VERSION) --json-out $(PACKAGE_REPORT_DIR)/packaging_audit.json --md-out $(PACKAGE_REPORT_DIR)/packaging_audit.md

cpack-smoke:
	rm -rf $(CPACK_BUILD_DIR)
	$(CMAKE) -S . -B $(CPACK_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DBMP_BUILD_TESTS=OFF -DBMP_BUILD_TOOLS=OFF
	$(CMAKE) --build $(CPACK_BUILD_DIR)
	cd $(CPACK_BUILD_DIR) && $(CPACK) -G TGZ && $(CPACK) -G ZIP && $(CPACK) -G TGZ --config CPackSourceConfig.cmake
	$(PYTHON) ./scripts/audit_packaging.py --repo-root . --project $(PROJECT_NAME) --version $(VERSION) --build-dir $(CPACK_BUILD_DIR) --json-out $(PACKAGE_REPORT_DIR)/cpack_audit.json --md-out $(PACKAGE_REPORT_DIR)/cpack_audit.md

tests/fuzz_bmp_c89_file: tests/fuzz_c89_file.c tests/fuzz_c89_common.h $(SRC)
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I./bmp -I./include -o $@ tests/fuzz_c89_file.c $(SRC)

tests/fuzz_bmp_c89_stdin: tests/fuzz_c89_stdin.c tests/fuzz_c89_common.h $(SRC)
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I./bmp -I./include -o $@ tests/fuzz_c89_stdin.c $(SRC)

tests/fuzz_bmp_parse_file: tests/fuzz_c89_file.c tests/fuzz_c89_common.h $(SRC)
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -DBMP_FUZZ_MODE=BMP_FUZZ_MODE_PARSE -I./bmp -I./include -o $@ tests/fuzz_c89_file.c $(SRC)

tests/fuzz_bmp_decode_file: tests/fuzz_c89_file.c tests/fuzz_c89_common.h $(SRC)
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -DBMP_FUZZ_MODE=BMP_FUZZ_MODE_DECODE -I./bmp -I./include -o $@ tests/fuzz_c89_file.c $(SRC)

tests/fuzz_bmp_payload_file: tests/fuzz_c89_file.c tests/fuzz_c89_common.h $(SRC)
	$(CC) $(CFLAGS) -DBMP_STATIC_DEFINE -DBMP_FUZZ_MODE=BMP_FUZZ_MODE_PAYLOAD -I./bmp -I./include -o $@ tests/fuzz_c89_file.c $(SRC)

tests/fuzz_bmp_afl: tests/fuzz_c89_stdin.c tests/fuzz_c89_common.h $(SRC)
	$(AFL_CC) $(CFLAGS) -DBMP_STATIC_DEFINE -I./bmp -I./include -o $@ tests/fuzz_c89_stdin.c $(SRC)

test: all tests/test_smoke protocol-audit
	./tests/test_smoke

test-sanitize: test

protocol-audit:
	$(PYTHON) ./scripts/audit_c89_protocol.py --repo-root . --json-out verification/c89_protocol_audit.json

fuzz-c89: tests/fuzz_bmp_c89_file tests/fuzz_bmp_c89_stdin

fuzz-c89-modes: tests/fuzz_bmp_parse_file tests/fuzz_bmp_decode_file tests/fuzz_bmp_payload_file

fuzz-stdin: tests/fuzz_bmp_c89_stdin

fuzz-afl: tests/fuzz_bmp_afl

fuzz-libfuzzer: fuzz-c89
	@echo "libFuzzer ABI adapter intentionally omitted by the strict C89/no-wide-size protocol; C89 file/stdin harnesses are built instead."

fuzz-persistent: fuzz-c89
	@echo "Use an AFL compiler with tests/fuzz_c89_stdin.c for persistent/external fuzz orchestration."

fuzz-afl-persistent: fuzz-afl

fuzz-libfuzzer-modes: fuzz-c89-modes
	@echo "Strict-protocol mode harnesses built without the libFuzzer ABI adapter."

fuzz-stdin-modes: fuzz-c89-modes

seedcheck-stdin: tests/fuzz_bmp_c89_file
	$(PYTHON) ./scripts/run_c89_corpus.py --runner tests/fuzz_bmp_c89_file --corpus $(CORPUS_DIR) --json-out tests/fuzz_reports/c89_corpus.json

seedcheck-stdin-parse: tests/fuzz_bmp_parse_file
	@for f in $(CORPUS_DIR)/*; do test -f $$f || continue; case "$$f" in *README.txt) continue;; esac; ./tests/fuzz_bmp_parse_file $$f >/dev/null || exit 1; done

seedcheck-stdin-decode: tests/fuzz_bmp_decode_file
	@for f in $(CORPUS_DIR)/*; do test -f $$f || continue; case "$$f" in *README.txt) continue;; esac; ./tests/fuzz_bmp_decode_file $$f >/dev/null || exit 1; done

seedcheck-stdin-payload: tests/fuzz_bmp_payload_file
	@for f in $(CORPUS_DIR)/*; do test -f $$f || continue; case "$$f" in *README.txt) continue;; esac; ./tests/fuzz_bmp_payload_file $$f >/dev/null || exit 1; done

seedcheck-stdin-modes: seedcheck-stdin-parse seedcheck-stdin-decode seedcheck-stdin-payload

seedcheck-libfuzzer: seedcheck-stdin
seedcheck-libfuzzer-parse: seedcheck-stdin-parse
seedcheck-libfuzzer-decode: seedcheck-stdin-decode
seedcheck-libfuzzer-payload: seedcheck-stdin-payload
seedcheck-libfuzzer-modes: seedcheck-stdin-modes

persistent-check: seedcheck-stdin

corpus-min-afl: tests/fuzz_bmp_afl
	sh ./scripts/minimize_corpus_afl.sh ./tests/fuzz_bmp_afl "$(CORPUS_DIR)" "$(MIN_CORPUS_DIR_AFL)"

corpus-tmin-afl: tests/fuzz_bmp_afl
	sh ./scripts/tmin_corpus_afl.sh ./tests/fuzz_bmp_afl "$(MIN_CORPUS_DIR_AFL)" "$(MIN_CORPUS_DIR_AFL_TMIN)"

corpus-min-libfuzzer corpus-min-libfuzzer-parse corpus-min-libfuzzer-decode corpus-min-libfuzzer-payload:
	@echo "libFuzzer minimization is not enabled under the strict no-wide-size ABI; use AFL minimization or scripts/run_c89_corpus.py."

crash-triage-stdin: tests/fuzz_bmp_c89_file
	$(PYTHON) ./scripts/triage_crashes.py --mode file --harness ./tests/fuzz_bmp_c89_file --input-dir "$(CRASH_DIR)" --output-dir "$(TRIAGE_DIR_STDIN)" --timeout "$(TRIAGE_TIMEOUT)"

crash-triage-libfuzzer: crash-triage-stdin
	@echo "triage performed with the strict C89 file harness."

crash-buckets-libfuzzer: crash-triage-libfuzzer
crash-buckets-stdin: crash-triage-stdin

repro-stdin: tests/fuzz_bmp_c89_file
	sh ./scripts/repro_crash.sh file ./tests/fuzz_bmp_c89_file "$(FILE)"

repro-libfuzzer: repro-stdin
	@echo "reproduced with the strict C89 file harness."

bundle-ci-parse bundle-ci-decode bundle-ci-payload:
	@echo "Use tests/fuzz_reports/c89_corpus.json plus triage output as strict-protocol CI fuzz artifacts."

ci-fuzz-smoke: test seedcheck-stdin seedcheck-stdin-modes

test-round7:
	$(PYTHON) ./tests/test_round7_tools.py

test-round9: test

test-round10: test tools/bmp_diag_cli
	./tools/bmp_diag_cli --json tests/fuzz_corpus/valid_rgb24.bmp >/dev/null
	./tools/bmp_diag_cli --text tests/fuzz_corpus/payload_png.bmp >/dev/null

test-round11: test-round10 preset-smoke portability-audit install-api-smoke api-surface-audit

release-check: test-round11 seedcheck-stdin-modes

clean:
	rm -f $(OBJ) libbmp.a libbmp.so $(SHARED_SONAME) $(SHARED_REAL) $(BMP_PC) tests/test_smoke tools/bmp_diag_cli tools/bmp_compat_cli tools/bmp_bench
	rm -f tests/fuzz_bmp_c89_file tests/fuzz_bmp_c89_stdin tests/fuzz_bmp_afl
	rm -f tests/fuzz_bmp_parse_file tests/fuzz_bmp_decode_file tests/fuzz_bmp_payload_file
	rm -rf $(TRIAGE_DIR_LIBFUZZER) $(TRIAGE_DIR_STDIN) $(MIN_CORPUS_DIR) $(MIN_CORPUS_DIR_AFL) $(MIN_CORPUS_DIR_AFL_TMIN)
	rm -rf $(MIN_CORPUS_DIR_PARSE) $(MIN_CORPUS_DIR_DECODE) $(MIN_CORPUS_DIR_PAYLOAD) $(CI_ARTIFACTS_DIR) $(CI_LOG_DIR)
	rm -rf $(CMAKE_BUILD_DIR) $(CMAKE_STAGE_DIR) $(CMAKE_CONSUMER_BUILD_DIR) $(CPACK_BUILD_DIR) $(PKGCONFIG_STAGE_DIR) $(PKGCONFIG_CONSUMER_BUILD_DIR) $(PACKAGE_REPORT_DIR) $(PORTABILITY_REPORT_DIR) $(API_STAGE_DIR) $(API_REPORT_DIR) $(ANALYZER_REPORT_DIR) $(HEADER_BUILD_DIR) $(COMPILER_CONTRACT_REPORT_DIR) build/presets
	rm -f tests/test_public_api

.PHONY: ruleset-render governance-audit governance-plan governance-apply all shared clean test test-sanitize test-round7 test-round9 test-round10 ecosystem-audit pkgconfig-smoke cpack-smoke release-check diag-cli benchmark install uninstall pkgconfig ecosystem-audit pkgconfig-smoke cpack-smoke 	fuzz-libfuzzer fuzz-stdin fuzz-afl fuzz-persistent fuzz-afl-persistent 	fuzz-libfuzzer-modes fuzz-stdin-modes 	seedcheck-libfuzzer seedcheck-stdin seedcheck-libfuzzer-modes seedcheck-stdin-modes 	seedcheck-libfuzzer-parse seedcheck-libfuzzer-decode seedcheck-libfuzzer-payload 	seedcheck-stdin-parse seedcheck-stdin-decode seedcheck-stdin-payload 	persistent-check corpus-min-libfuzzer corpus-min-afl corpus-tmin-afl 	corpus-min-libfuzzer-parse corpus-min-libfuzzer-decode corpus-min-libfuzzer-payload 	crash-triage-libfuzzer crash-triage-stdin crash-buckets-libfuzzer crash-buckets-stdin repro-libfuzzer repro-stdin 	bundle-ci-parse bundle-ci-decode bundle-ci-payload ci-fuzz-smoke abi-check test-asan test-ubsan test-sanitizers test-hardened cmake-smoke policy-audit preset-smoke portability-audit install-api-smoke api-surface-audit


test-round12: test-round11 visibility-audit
