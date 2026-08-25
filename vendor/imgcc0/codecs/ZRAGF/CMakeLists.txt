cmake_minimum_required(VERSION 3.16)
project(zragflib VERSION 0.70.0 LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(ZRAGF_ENABLE_STRICT "Enable stricter warnings" ON)
option(ZRAGF_ENABLE_ASAN "Enable Address/UB sanitizers" OFF)
option(ZRAGF_BUILD_TESTS "Build test executables" ON)
option(ZRAGF_BUILD_FUZZERS "Build libFuzzer harnesses (Clang only)" OFF)
option(ZRAGF_ENABLE_C89 "Build library in strict C89/C90 mode" ON)
option(ZRAGF_BUILD_PUBLIC_BENCH "Build the public comparative benchmark suite" ON)
option(ZRAGF_ENABLE_MINIZ_BENCH "Enable optional miniz comparator in the public benchmark when miniz is available" ON)
set(ZRAGF_MINIZ_ROOT "" CACHE PATH "Path to an optional miniz source drop or install prefix")

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(ZRAGF_SOURCES
    zragflib_internal.c
    zragf_bitio.c
    zragf_compress.c
    zragf_decompress.c
    zragf_deflate_rfc1951.c
    zragf_frame.c
    zragf_huff.c
    zragf_lz.c
    zragf_stream.c
    zragf_api.c
    zragf_workspace.c
    zragf_protocol89_mem.c

    zragf_deflate/deflate_blocks.c
    zragf_deflate/deflate_core.c
    zragf_deflate/deflate_cost.c
    zragf_deflate/deflate_emit.c
    zragf_deflate/deflate_fixed.c
    zragf_deflate/deflate_hash.c
    zragf_deflate/deflate_huffman.c
    zragf_deflate/deflate_huffman_shared.c
    zragf_deflate/deflate_lazy.c
    zragf_deflate/deflate_matcher.c
    zragf_deflate/deflate_plan.c
    zragf_deflate/deflate_rle.c
    zragf_deflate/deflate_stream.c
    zragf_deflate/deflate_tokens.c

    zragf_inflate/inflate_core.c
    zragf_inflate/inflate_huffman.c
    zragf_inflate/inflate_shared.c
    zragf_inflate/inflate_stream.c
)

add_library(zragflib STATIC ${ZRAGF_SOURCES})

target_include_directories(zragflib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/zragf_deflate>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/zragf_inflate>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(zragf::zragflib ALIAS zragflib)

set_target_properties(zragflib PROPERTIES
    EXPORT_NAME zragflib
    PUBLIC_HEADER zragflib.h
)

if (ZRAGF_ENABLE_STRICT)
    if (MSVC)
        target_compile_options(zragflib PRIVATE /W4)
    else()
        target_compile_options(zragflib PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            -Wstrict-prototypes -Wmissing-prototypes
        )
    endif()
endif()

if (ZRAGF_ENABLE_ASAN AND NOT MSVC)
    target_compile_options(zragflib PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(zragflib PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

if (ZRAGF_BUILD_TESTS)
    enable_testing()

    add_executable(zragf_test_native tests/test_native_roundtrip.c)
    target_link_libraries(zragf_test_native PRIVATE zragflib)
    add_test(NAME zragf_test_native COMMAND zragf_test_native)

    add_executable(zragf_test_wrappers tests/test_wrapper_roundtrip.c)
    target_link_libraries(zragf_test_wrappers PRIVATE zragflib)
    add_test(NAME zragf_test_wrappers COMMAND zragf_test_wrappers)

    add_executable(zragf_test_corruption tests/test_corruption.c)
    target_link_libraries(zragf_test_corruption PRIVATE zragflib)
    add_test(NAME zragf_test_corruption COMMAND zragf_test_corruption)

    add_executable(zragf_test_phase2 tests/test_phase2_deflate.c)
    target_link_libraries(zragf_test_phase2 PRIVATE zragflib)
    add_test(NAME zragf_test_phase2 COMMAND zragf_test_phase2)

    add_executable(zragf_test_external_vectors tests/test_external_vectors.c)
    target_link_libraries(zragf_test_external_vectors PRIVATE zragflib)
    add_test(NAME zragf_test_external_vectors COMMAND zragf_test_external_vectors)

    add_executable(zragf_test_phase64_alloc_workspace tests/test_phase64_alloc_workspace.c)
    target_link_libraries(zragf_test_phase64_alloc_workspace PRIVATE zragflib)
    add_test(NAME zragf_test_phase64_alloc_workspace COMMAND zragf_test_phase64_alloc_workspace)

    add_executable(zragf_test_phase65_wrapper_workspace tests/test_phase65_wrapper_workspace.c)
    target_link_libraries(zragf_test_phase65_wrapper_workspace PRIVATE zragflib)
    add_test(NAME zragf_test_phase65_wrapper_workspace COMMAND zragf_test_phase65_wrapper_workspace)

    add_executable(zragf_test_phase66_c89_smoke tests/test_phase66_c89_smoke.c)
    target_link_libraries(zragf_test_phase66_c89_smoke PRIVATE zragflib)
    add_test(NAME zragf_test_phase66_c89_smoke COMMAND zragf_test_phase66_c89_smoke)

    add_executable(zragf_test_phase3_dynamic tests/test_phase3_dynamic.c)
    target_link_libraries(zragf_test_phase3_dynamic PRIVATE zragflib)
    add_test(NAME zragf_test_phase3_dynamic COMMAND zragf_test_phase3_dynamic)

    add_executable(zragf_phase3_emit_case tests/phase3_emit_case.c)
    target_link_libraries(zragf_phase3_emit_case PRIVATE zragflib)

    add_executable(zragf_protocol89_byte_emitter tests/protocol89_byte_emitter.c)
    target_link_libraries(zragf_protocol89_byte_emitter PRIVATE zragflib)

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if (Python3_Interpreter_FOUND)
        add_test(
            NAME zragf_test_phase3_external_python
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/validate_phase3_external.py $<TARGET_FILE:zragf_phase3_emit_case>
        )
        add_test(
            NAME zragf_test_protocol89_byte_exact
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/validate_protocol89_byte_exact.py $<TARGET_FILE:zragf_protocol89_byte_emitter> ${CMAKE_CURRENT_SOURCE_DIR}/tests/golden/protocol89_original_outputs.bin
        )
        add_test(
            NAME zragf_test_protocol89_source_audit
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/protocol89_audit.py
        )
    endif()
endif()

find_package(ZLIB QUIET)
find_path(LIBDEFLATE_INCLUDE_DIR libdeflate.h)
find_library(LIBDEFLATE_LIBRARY NAMES deflate libdeflate)
if (LIBDEFLATE_INCLUDE_DIR AND LIBDEFLATE_LIBRARY)
    add_library(libdeflate::libdeflate UNKNOWN IMPORTED)
    set_target_properties(libdeflate::libdeflate PROPERTIES
        IMPORTED_LOCATION "${LIBDEFLATE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBDEFLATE_INCLUDE_DIR}")
    set(ZRAGF_HAVE_LIBDEFLATE ON)
else()
    set(ZRAGF_HAVE_LIBDEFLATE OFF)
endif()

set(ZRAGF_HAVE_MINIZ OFF)
if (ZRAGF_ENABLE_MINIZ_BENCH)
    set(_zragf_miniz_source_dir "")
    if (ZRAGF_MINIZ_ROOT AND EXISTS "${ZRAGF_MINIZ_ROOT}/miniz.c" AND EXISTS "${ZRAGF_MINIZ_ROOT}/miniz.h")
        set(_zragf_miniz_source_dir "${ZRAGF_MINIZ_ROOT}")
    elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz/miniz.c" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz/miniz.h")
        set(_zragf_miniz_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz")
    endif()

    if (_zragf_miniz_source_dir)
        add_library(zragf_miniz_vendor STATIC "${_zragf_miniz_source_dir}/miniz.c")
        target_include_directories(zragf_miniz_vendor PUBLIC "${_zragf_miniz_source_dir}")
        if (MSVC)
            target_compile_options(zragf_miniz_vendor PRIVATE /W0)
        else()
            target_compile_options(zragf_miniz_vendor PRIVATE -w)
        endif()
        add_library(miniz::miniz ALIAS zragf_miniz_vendor)
        set(ZRAGF_HAVE_MINIZ ON)
    else()
        find_path(MINIZ_INCLUDE_DIR NAMES miniz.h HINTS ${ZRAGF_MINIZ_ROOT} PATH_SUFFIXES include .)
        find_library(MINIZ_LIBRARY NAMES miniz HINTS ${ZRAGF_MINIZ_ROOT} PATH_SUFFIXES lib .)
        if (MINIZ_INCLUDE_DIR AND MINIZ_LIBRARY)
            add_library(miniz::miniz UNKNOWN IMPORTED)
            set_target_properties(miniz::miniz PROPERTIES
                IMPORTED_LOCATION "${MINIZ_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${MINIZ_INCLUDE_DIR}")
            set(ZRAGF_HAVE_MINIZ ON)
        endif()
    endif()
endif()
if (ZLIB_FOUND)
    if (ZRAGF_BUILD_TESTS)
        add_executable(zragf_test_phase4_tuning tests/test_phase4_tuning.c)
        target_link_libraries(zragf_test_phase4_tuning PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase4_tuning COMMAND zragf_test_phase4_tuning)

        add_executable(zragf_test_phase5_streaming tests/test_phase5_streaming.c)
        target_link_libraries(zragf_test_phase5_streaming PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase5_streaming COMMAND zragf_test_phase5_streaming)

        add_executable(zragf_test_phase5_incremental_inflate tests/test_phase5_incremental_inflate.c)
        target_link_libraries(zragf_test_phase5_incremental_inflate PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase5_incremental_inflate COMMAND zragf_test_phase5_incremental_inflate)

        add_executable(zragf_test_phase6_compat tests/test_phase6_compat.c)
        target_link_libraries(zragf_test_phase6_compat PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase6_compat COMMAND zragf_test_phase6_compat)

        add_executable(zragf_test_phase7_headers_dict tests/test_phase7_headers_dict.c)
        target_link_libraries(zragf_test_phase7_headers_dict PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase7_headers_dict COMMAND zragf_test_phase7_headers_dict)

        add_executable(zragf_test_phase8_copy_prime_sync tests/test_phase8_copy_prime_sync.c)
        target_link_libraries(zragf_test_phase8_copy_prime_sync PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase8_copy_prime_sync COMMAND zragf_test_phase8_copy_prime_sync)

        add_executable(zragf_test_phase9_params_validate tests/test_phase9_params_validate.c)
        target_link_libraries(zragf_test_phase9_params_validate PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase9_params_validate COMMAND zragf_test_phase9_params_validate)
        add_executable(zragf_test_phase10_backend_running tests/test_phase10_backend_running.c)
        target_link_libraries(zragf_test_phase10_backend_running PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase10_backend_running COMMAND zragf_test_phase10_backend_running)
        add_executable(zragf_test_phase11_splitter tests/test_phase11_splitter.c)
        target_link_libraries(zragf_test_phase11_splitter PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase11_splitter COMMAND zragf_test_phase11_splitter)
        add_executable(zragf_test_phase12_unified_tokenizer tests/test_phase12_unified_tokenizer.c)
        target_link_libraries(zragf_test_phase12_unified_tokenizer PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase12_unified_tokenizer COMMAND zragf_test_phase12_unified_tokenizer)
        add_executable(zragf_test_phase13_unified_emitter tests/test_phase13_unified_emitter.c)
        target_link_libraries(zragf_test_phase13_unified_emitter PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase13_unified_emitter COMMAND zragf_test_phase13_unified_emitter)
        add_executable(zragf_test_phase14_cost_model tests/test_phase14_cost_model.c)
        target_link_libraries(zragf_test_phase14_cost_model PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase14_cost_model COMMAND zragf_test_phase14_cost_model)
        add_executable(zragf_test_phase15_cost_legacy tests/test_phase15_cost_legacy.c)
        target_link_libraries(zragf_test_phase15_cost_legacy PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase15_cost_legacy COMMAND zragf_test_phase15_cost_legacy)
        add_executable(zragf_test_phase16_backend_hardening tests/test_phase16_backend_hardening.c)
        target_link_libraries(zragf_test_phase16_backend_hardening PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase16_backend_hardening COMMAND zragf_test_phase16_backend_hardening)
        add_executable(zragf_test_phase17_inflate_hardening tests/test_phase17_inflate_hardening.c)
        target_link_libraries(zragf_test_phase17_inflate_hardening PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase17_inflate_hardening COMMAND zragf_test_phase17_inflate_hardening)
        add_executable(zragf_test_phase18_fuzz_regressions tests/test_phase18_fuzz_regressions.c)
        target_link_libraries(zragf_test_phase18_fuzz_regressions PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase18_fuzz_regressions COMMAND zragf_test_phase18_fuzz_regressions)
        if (ZRAGF_HAVE_LIBDEFLATE)
            add_executable(zragf_test_phase19_differential tests/test_phase19_differential.c)
            target_link_libraries(zragf_test_phase19_differential PRIVATE zragflib ZLIB::ZLIB libdeflate::libdeflate)
            add_test(NAME zragf_test_phase19_differential COMMAND zragf_test_phase19_differential)
        endif()
        add_executable(zragf_test_phase20_advantage tests/test_phase20_advantage.c)
        target_link_libraries(zragf_test_phase20_advantage PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase20_advantage COMMAND zragf_test_phase20_advantage)
        add_executable(zragf_test_phase21_text_models tests/test_phase21_text_models.c)
        target_link_libraries(zragf_test_phase21_text_models PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase21_text_models COMMAND zragf_test_phase21_text_models)
        add_executable(zragf_test_phase22_text_speed tests/test_phase22_text_speed.c)
        target_link_libraries(zragf_test_phase22_text_speed PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase22_text_speed COMMAND zragf_test_phase22_text_speed)
        add_executable(zragf_test_phase23_fastpath tests/test_phase23_fastpath.c)
        target_link_libraries(zragf_test_phase23_fastpath PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase23_fastpath COMMAND zragf_test_phase23_fastpath)
        add_executable(zragf_test_phase29_step29 tests/test_phase29_step29.c)
        target_link_libraries(zragf_test_phase29_step29 PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase29_step29 COMMAND zragf_test_phase29_step29)
        add_executable(zragf_test_phase30_wholebuf tests/test_phase30_wholebuf.c)
        target_link_libraries(zragf_test_phase30_wholebuf PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase30_wholebuf COMMAND zragf_test_phase30_wholebuf)
        add_executable(zragf_test_phase31_emit_stats tests/test_phase31_emit_stats.c)
        target_link_libraries(zragf_test_phase31_emit_stats PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase31_emit_stats COMMAND zragf_test_phase31_emit_stats)
        add_executable(zragf_test_phase32_matcher_tokens tests/test_phase32_matcher_tokens.c)
        target_link_libraries(zragf_test_phase32_matcher_tokens PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase32_matcher_tokens COMMAND zragf_test_phase32_matcher_tokens)
        add_executable(zragf_test_phase33_split_model tests/test_phase33_split_model.c)
        target_link_libraries(zragf_test_phase33_split_model PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase33_split_model COMMAND zragf_test_phase33_split_model)
        add_executable(zragf_test_phase35_partition_budget tests/test_phase35_partition_budget.c)
        target_link_libraries(zragf_test_phase35_partition_budget PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase35_partition_budget COMMAND zragf_test_phase35_partition_budget)
        add_executable(zragf_test_phase36_estimate_cache tests/test_phase36_estimate_cache.c)
        target_link_libraries(zragf_test_phase36_estimate_cache PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase36_estimate_cache COMMAND zragf_test_phase36_estimate_cache)
        add_executable(zragf_test_phase37_winner_emit tests/test_phase37_winner_emit.c)
        target_link_libraries(zragf_test_phase37_winner_emit PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase37_winner_emit COMMAND zragf_test_phase37_winner_emit)

        add_executable(zragf_test_phase38_prepared_cache tests/test_phase38_prepared_cache.c)
        target_link_libraries(zragf_test_phase38_prepared_cache PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase38_prepared_cache COMMAND zragf_test_phase38_prepared_cache)
        add_executable(zragf_test_phase39_partition_state tests/test_phase39_partition_state.c)
        target_link_libraries(zragf_test_phase39_partition_state PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase39_partition_state COMMAND zragf_test_phase39_partition_state)
        add_executable(zragf_test_phase40_dual_cache tests/test_phase40_dual_cache.c)
        target_link_libraries(zragf_test_phase40_dual_cache PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase40_dual_cache COMMAND zragf_test_phase40_dual_cache)
        add_executable(zragf_test_phase41_winner_dynamic tests/test_phase41_winner_dynamic.c)
        target_link_libraries(zragf_test_phase41_winner_dynamic PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase41_winner_dynamic COMMAND zragf_test_phase41_winner_dynamic)
        add_executable(zragf_test_phase42_public_emit_cache tests/test_phase42_public_emit_cache.c)
        target_link_libraries(zragf_test_phase42_public_emit_cache PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase42_public_emit_cache COMMAND zragf_test_phase42_public_emit_cache)
        add_executable(zragf_test_phase43_suffix_tail_cache tests/test_phase43_suffix_tail_cache.c)
        target_link_libraries(zragf_test_phase43_suffix_tail_cache PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase43_suffix_tail_cache COMMAND zragf_test_phase43_suffix_tail_cache)
        add_executable(zragf_test_phase44_tail_bounds tests/test_phase44_tail_bounds.c)
        target_link_libraries(zragf_test_phase44_tail_bounds PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase44_tail_bounds COMMAND zragf_test_phase44_tail_bounds)
        add_executable(zragf_test_phase45_tail_lower_bound tests/test_phase45_tail_lower_bound.c)
        target_link_libraries(zragf_test_phase45_tail_lower_bound PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase45_tail_lower_bound COMMAND zragf_test_phase45_tail_lower_bound)
        add_executable(zragf_test_phase46_candidate_order tests/test_phase46_candidate_order.c)
        target_link_libraries(zragf_test_phase46_candidate_order PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase46_candidate_order COMMAND zragf_test_phase46_candidate_order)
        add_executable(zragf_test_phase47_tail_seed tests/test_phase47_tail_seed.c)
        target_link_libraries(zragf_test_phase47_tail_seed PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase47_tail_seed COMMAND zragf_test_phase47_tail_seed)
        add_executable(zragf_test_phase48_tail_lb_query tests/test_phase48_tail_lb_query.c)
        target_link_libraries(zragf_test_phase48_tail_lb_query PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase48_tail_lb_query COMMAND zragf_test_phase48_tail_lb_query)
        add_executable(zragf_test_phase49_suffix_memo tests/test_phase49_suffix_memo.c)
        target_link_libraries(zragf_test_phase49_suffix_memo PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase49_suffix_memo COMMAND zragf_test_phase49_suffix_memo)
        add_executable(zragf_test_phase50_tail_priority tests/test_phase50_tail_priority.c)
        target_link_libraries(zragf_test_phase50_tail_priority PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase50_tail_priority COMMAND zragf_test_phase50_tail_priority)
        add_executable(zragf_test_phase51_lb_prune tests/test_phase51_lb_prune.c)
        target_link_libraries(zragf_test_phase51_lb_prune PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase51_lb_prune COMMAND zragf_test_phase51_lb_prune)
        add_executable(zragf_test_phase52_exact_tail_cache tests/test_phase52_exact_tail_cache.c)
        target_link_libraries(zragf_test_phase52_exact_tail_cache PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase52_exact_tail_cache COMMAND zragf_test_phase52_exact_tail_cache)
        add_executable(zragf_test_phase53_exact_tail_query tests/test_phase53_exact_tail_query.c)
        target_link_libraries(zragf_test_phase53_exact_tail_query PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase53_exact_tail_query COMMAND zragf_test_phase53_exact_tail_query)
        add_executable(zragf_test_phase54_lb_query_cache tests/test_phase54_lb_query_cache.c)
        target_link_libraries(zragf_test_phase54_lb_query_cache PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase54_lb_query_cache COMMAND zragf_test_phase54_lb_query_cache)
        add_executable(zragf_test_phase55_suffix_query_cache tests/test_phase55_suffix_query_cache.c)
        target_link_libraries(zragf_test_phase55_suffix_query_cache PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase55_suffix_query_cache COMMAND zragf_test_phase55_suffix_query_cache)
        add_executable(zragf_test_phase56_slot_hint_maps tests/test_phase56_slot_hint_maps.c)
        target_link_libraries(zragf_test_phase56_slot_hint_maps PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase56_slot_hint_maps COMMAND zragf_test_phase56_slot_hint_maps)
        add_executable(zragf_test_phase57_prepared_donor_hints tests/test_phase57_prepared_donor_hints.c)
        target_link_libraries(zragf_test_phase57_prepared_donor_hints PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase57_prepared_donor_hints COMMAND zragf_test_phase57_prepared_donor_hints)
        add_executable(zragf_test_phase58_prepared_donor_miss_hints tests/test_phase58_prepared_donor_miss_hints.c)
        target_link_libraries(zragf_test_phase58_prepared_donor_miss_hints PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase58_prepared_donor_miss_hints COMMAND zragf_test_phase58_prepared_donor_miss_hints)
        add_executable(zragf_test_phase59_query_assoc tests/test_phase59_query_assoc.c)
        target_link_libraries(zragf_test_phase59_query_assoc PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase59_query_assoc COMMAND zragf_test_phase59_query_assoc)
        add_executable(zragf_test_phase60_slot_hint_assoc tests/test_phase60_slot_hint_assoc.c)
        target_link_libraries(zragf_test_phase60_slot_hint_assoc PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase60_slot_hint_assoc COMMAND zragf_test_phase60_slot_hint_assoc)
        add_executable(zragf_test_phase61_miss_paths tests/test_phase61_miss_paths.c)
        target_link_libraries(zragf_test_phase61_miss_paths PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase61_miss_paths COMMAND zragf_test_phase61_miss_paths)
        add_executable(zragf_test_phase62_dual_prune_miss tests/test_phase62_dual_prune_miss.c)
        target_link_libraries(zragf_test_phase62_dual_prune_miss PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase62_dual_prune_miss COMMAND zragf_test_phase62_dual_prune_miss)
        add_executable(zragf_test_phase63_prepared_donor_query tests/test_phase63_prepared_donor_query.c)
        target_link_libraries(zragf_test_phase63_prepared_donor_query PRIVATE zragflib ZLIB::ZLIB)
        add_test(NAME zragf_test_phase63_prepared_donor_query COMMAND zragf_test_phase63_prepared_donor_query)
    endif()
    add_executable(zragf_bench_compare_zlib bench/bench_compare_zlib.c)
    target_link_libraries(zragf_bench_compare_zlib PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_streaming_compare bench/bench_streaming_compare.c)
    target_link_libraries(zragf_bench_streaming_compare PRIVATE zragflib ZLIB::ZLIB)
    if (ZRAGF_HAVE_LIBDEFLATE)
        add_executable(zragf_bench_phase19_diff bench/bench_phase19_diff.c)
        target_link_libraries(zragf_bench_phase19_diff PRIVATE zragflib ZLIB::ZLIB libdeflate::libdeflate)
    endif()
    add_executable(zragf_bench_phase20_advantage bench/bench_phase20_advantage.c)
    target_link_libraries(zragf_bench_phase20_advantage PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase21_text bench/bench_phase21_text.c)
    target_link_libraries(zragf_bench_phase21_text PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase22_speed_text bench/bench_phase22_speed_text.c)
    target_link_libraries(zragf_bench_phase22_speed_text PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase23_speed_text bench/bench_phase23_speed_text.c)
    target_link_libraries(zragf_bench_phase23_speed_text PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase29_step29 bench/bench_phase29_step29.c)
    target_link_libraries(zragf_bench_phase29_step29 PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase30_wholebuf bench/bench_phase30_wholebuf.c)
    target_link_libraries(zragf_bench_phase30_wholebuf PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase31_emit_stats bench/bench_phase31_emit_stats.c)
    target_link_libraries(zragf_bench_phase31_emit_stats PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase32_matcher_tokens bench/bench_phase32_matcher_tokens.c)
    target_link_libraries(zragf_bench_phase32_matcher_tokens PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase33_split_model bench/bench_phase33_split_model.c)
    target_link_libraries(zragf_bench_phase33_split_model PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase35_partition_budget bench/bench_phase35_partition_budget.c)
    target_link_libraries(zragf_bench_phase35_partition_budget PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase36_estimate_cache bench/bench_phase36_estimate_cache.c)
    target_link_libraries(zragf_bench_phase36_estimate_cache PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase37_winner_emit bench/bench_phase37_winner_emit.c)
    target_link_libraries(zragf_bench_phase37_winner_emit PRIVATE zragflib ZLIB::ZLIB)

    add_executable(zragf_bench_phase38_prepared_cache bench/bench_phase38_prepared_cache.c)
    target_link_libraries(zragf_bench_phase38_prepared_cache PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase39_partition_state bench/bench_phase39_partition_state.c)
    target_link_libraries(zragf_bench_phase39_partition_state PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase40_dual_cache bench/bench_phase40_dual_cache.c)
    target_link_libraries(zragf_bench_phase40_dual_cache PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase41_winner_dynamic bench/bench_phase41_winner_dynamic.c)
    target_link_libraries(zragf_bench_phase41_winner_dynamic PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase42_public_emit_cache bench/bench_phase42_public_emit_cache.c)
    target_link_libraries(zragf_bench_phase42_public_emit_cache PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase43_suffix_tail_cache bench/bench_phase43_suffix_tail_cache.c)
    target_link_libraries(zragf_bench_phase43_suffix_tail_cache PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase44_tail_bounds bench/bench_phase44_tail_bounds.c)
    target_link_libraries(zragf_bench_phase44_tail_bounds PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase45_tail_lower_bound bench/bench_phase45_tail_lower_bound.c)
    target_link_libraries(zragf_bench_phase45_tail_lower_bound PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase46_candidate_order bench/bench_phase46_candidate_order.c)
    target_link_libraries(zragf_bench_phase46_candidate_order PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase47_tail_seed bench/bench_phase47_tail_seed.c)
    target_link_libraries(zragf_bench_phase47_tail_seed PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase48_tail_lb_query bench/bench_phase48_tail_lb_query.c)
    target_link_libraries(zragf_bench_phase48_tail_lb_query PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase49_suffix_memo bench/bench_phase49_suffix_memo.c)
    target_link_libraries(zragf_bench_phase49_suffix_memo PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase50_tail_priority bench/bench_phase50_tail_priority.c)
    target_link_libraries(zragf_bench_phase50_tail_priority PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase51_lb_prune bench/bench_phase51_lb_prune.c)
    target_link_libraries(zragf_bench_phase51_lb_prune PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase52_exact_tail_cache bench/bench_phase52_exact_tail_cache.c)
    target_link_libraries(zragf_bench_phase52_exact_tail_cache PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase53_exact_tail_query bench/bench_phase53_exact_tail_query.c)
    target_link_libraries(zragf_bench_phase53_exact_tail_query PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase54_lb_query_cache bench/bench_phase54_lb_query_cache.c)
    target_link_libraries(zragf_bench_phase54_lb_query_cache PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase55_suffix_query_cache bench/bench_phase55_suffix_query_cache.c)
    target_link_libraries(zragf_bench_phase55_suffix_query_cache PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase56_slot_hint_maps bench/bench_phase56_slot_hint_maps.c)
    target_link_libraries(zragf_bench_phase56_slot_hint_maps PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase57_prepared_donor_hints bench/bench_phase57_prepared_donor_hints.c)
    target_link_libraries(zragf_bench_phase57_prepared_donor_hints PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase58_prepared_donor_miss_hints bench/bench_phase58_prepared_donor_miss_hints.c)
    target_link_libraries(zragf_bench_phase58_prepared_donor_miss_hints PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase59_query_assoc bench/bench_phase59_query_assoc.c)
    target_link_libraries(zragf_bench_phase59_query_assoc PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase60_slot_hint_assoc bench/bench_phase60_slot_hint_assoc.c)
    target_link_libraries(zragf_bench_phase60_slot_hint_assoc PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase61_miss_paths bench/bench_phase61_miss_paths.c)
    target_link_libraries(zragf_bench_phase61_miss_paths PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase62_dual_prune_miss bench/bench_phase62_dual_prune_miss.c)
    target_link_libraries(zragf_bench_phase62_dual_prune_miss PRIVATE zragflib ZLIB::ZLIB)
    add_executable(zragf_bench_phase63_prepared_donor_query bench/bench_phase63_prepared_donor_query.c)
    target_link_libraries(zragf_bench_phase63_prepared_donor_query PRIVATE zragflib ZLIB::ZLIB)
    if (ZRAGF_BUILD_PUBLIC_BENCH)
        add_executable(zragf_bench_phase67_public bench/bench_phase67_public.c)
        target_link_libraries(zragf_bench_phase67_public PRIVATE zragflib ZLIB::ZLIB)
        if (ZRAGF_HAVE_LIBDEFLATE)
            target_link_libraries(zragf_bench_phase67_public PRIVATE libdeflate::libdeflate)
            target_compile_definitions(zragf_bench_phase67_public PRIVATE ZRAGF_HAVE_LIBDEFLATE_BENCH=1)
        endif()
    endif()
    if (ZRAGF_BUILD_PUBLIC_BENCH AND ZRAGF_BUILD_TESTS)
        add_test(NAME zragf_test_phase67_public_bench
                 COMMAND zragf_bench_phase67_public --quick --csv phase67_smoke.csv --summary phase67_smoke.md)
        set_tests_properties(zragf_test_phase67_public_bench PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    if (ZRAGF_BUILD_PUBLIC_BENCH)
        add_executable(zragf_bench_phase68_public bench/bench_phase68_public.c)
        target_link_libraries(zragf_bench_phase68_public PRIVATE zragflib ZLIB::ZLIB)
        if (ZRAGF_HAVE_LIBDEFLATE)
            target_link_libraries(zragf_bench_phase68_public PRIVATE libdeflate::libdeflate)
            target_compile_definitions(zragf_bench_phase68_public PRIVATE ZRAGF_HAVE_LIBDEFLATE_BENCH=1)
        endif()
        if (ZRAGF_HAVE_MINIZ)
            target_link_libraries(zragf_bench_phase68_public PRIVATE miniz::miniz)
            target_compile_definitions(zragf_bench_phase68_public PRIVATE ZRAGF_HAVE_MINIZ_BENCH=1)
        endif()

        add_executable(zragf_bench_phase69_public bench/bench_phase69_public.c)
        target_link_libraries(zragf_bench_phase69_public PRIVATE zragflib ZLIB::ZLIB)
        if (ZRAGF_HAVE_LIBDEFLATE)
            target_link_libraries(zragf_bench_phase69_public PRIVATE libdeflate::libdeflate)
            target_compile_definitions(zragf_bench_phase69_public PRIVATE ZRAGF_HAVE_LIBDEFLATE_BENCH=1)
        endif()
        if (ZRAGF_HAVE_MINIZ)
            target_link_libraries(zragf_bench_phase69_public PRIVATE miniz::miniz)
            target_compile_definitions(zragf_bench_phase69_public PRIVATE ZRAGF_HAVE_MINIZ_BENCH=1)
        endif()

        add_executable(zragf_bench_phase70_public bench/bench_phase70_public.c)
        target_link_libraries(zragf_bench_phase70_public PRIVATE zragflib ZLIB::ZLIB)
        if (ZRAGF_HAVE_LIBDEFLATE)
            target_link_libraries(zragf_bench_phase70_public PRIVATE libdeflate::libdeflate)
            target_compile_definitions(zragf_bench_phase70_public PRIVATE ZRAGF_HAVE_LIBDEFLATE_BENCH=1)
        endif()
        if (ZRAGF_HAVE_MINIZ)
            target_link_libraries(zragf_bench_phase70_public PRIVATE miniz::miniz)
            target_compile_definitions(zragf_bench_phase70_public PRIVATE ZRAGF_HAVE_MINIZ_BENCH=1)
        endif()
    endif()
    if (ZRAGF_BUILD_PUBLIC_BENCH AND ZRAGF_BUILD_TESTS AND Python3_Interpreter_FOUND)
        add_test(NAME zragf_test_phase68_public_manifest
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_phase68_public.py
                         --exe $<TARGET_FILE:zragf_bench_phase68_public>
                         --manifest ${CMAKE_CURRENT_SOURCE_DIR}/bench/corpora/phase68_local_smoke_manifest.json
                         --corpus-root ${CMAKE_CURRENT_SOURCE_DIR}/bench/corpora
                         --no-builtins --quick --out-dir ${CMAKE_CURRENT_BINARY_DIR}/phase68_smoke)
        set_tests_properties(zragf_test_phase68_public_manifest PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        add_test(NAME zragf_test_phase69_locked_corpora
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_phase69_locked_corpora.py
                         $<TARGET_FILE:zragf_bench_phase69_public>)
        set_tests_properties(zragf_test_phase69_locked_corpora PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        add_test(NAME zragf_test_phase70_materialize_cached
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_phase70_materialize_cached.py)
        set_tests_properties(zragf_test_phase70_materialize_cached PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        add_test(NAME zragf_test_phase70_locked_corpora
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_phase70_locked_corpora.py
                         $<TARGET_FILE:zragf_bench_phase70_public>)
        set_tests_properties(zragf_test_phase70_locked_corpora PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endif()

add_executable(zragf_bench_phase11_split_compare bench/bench_phase11_split_compare.c)
target_link_libraries(zragf_bench_phase11_split_compare PRIVATE zragflib)


if (ZRAGF_BUILD_FUZZERS AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_executable(zragf_fuzz_inflate fuzz/fuzz_inflate.c)
    target_link_libraries(zragf_fuzz_inflate PRIVATE zragflib)
    target_compile_options(zragf_fuzz_inflate PRIVATE
        -fsanitize=fuzzer-no-link,address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(zragf_fuzz_inflate PRIVATE
        -fsanitize=fuzzer,address,undefined
        -fno-omit-frame-pointer
    )
    if (ZLIB_FOUND)
        add_executable(zragf_fuzz_deflate fuzz/fuzz_deflate.c)
        target_link_libraries(zragf_fuzz_deflate PRIVATE zragflib ZLIB::ZLIB)
        if (ZRAGF_HAVE_LIBDEFLATE)
            target_link_libraries(zragf_fuzz_deflate PRIVATE libdeflate::libdeflate)
            target_compile_definitions(zragf_fuzz_deflate PRIVATE ZRAGF_HAVE_LIBDEFLATE=1)
        endif()
        target_compile_options(zragf_fuzz_deflate PRIVATE
            -fsanitize=fuzzer-no-link,address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(zragf_fuzz_deflate PRIVATE
            -fsanitize=fuzzer,address,undefined
            -fno-omit-frame-pointer
        )
    endif()
endif()

install(TARGETS zragflib
    EXPORT zragflibTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES README.md docs/api.md docs/design.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/zragflib
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/zragflibConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/zragflibConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zragflib
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/zragflibConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

install(EXPORT zragflibTargets
    FILE zragflibTargets.cmake
    NAMESPACE zragf::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zragflib
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/zragflibConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/zragflibConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zragflib
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/zragflib.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/zragflib.pc
               @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zragflib.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

add_executable(zragf_example_c89_simple examples/example_c89_simple.c)
target_link_libraries(zragf_example_c89_simple PRIVATE zragflib)
