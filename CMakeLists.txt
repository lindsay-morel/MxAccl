cmake_minimum_required(VERSION 3.13)

project(MX_API)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)
option(TEST_CASCADE "Option description" OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_compile_definitions(OS_LINUX)
endif()

get_filename_component(MX_API_HOME_DIR "." REALPATH)
message(STATUS "MX_API_HOME_DIR  set to " ${MX_API_HOME_DIR})
get_filename_component(MX_API_DIR "mx_accl" REALPATH)
message(STATUS "MX_API_DIR  set to " ${MX_API_DIR})
get_filename_component(MX_API_BUILD_DIR "build" REALPATH)
message(STATUS "MX_API_BUILD_DIR  set to " ${MX_API_BUILD_DIR})

if(DEFINED CMAKE_X86_FLAGS_BASE)
    set(CMAKE_X86_FLAGS_BASE "")
else()
    set(CMAKE_X86_FLAGS_BASE "-mtune=generic -mpopcnt -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mfxsr -mcx16 -msahf -mpclmul")
endif()
if(DEFINED CMAKE_X86_FLAGS_AVX2)
    set(CMAKE_X86_FLAGS_AVX2 "")
else()
    set(CMAKE_X86_FLAGS_AVX2 "-mavx -mavx2 -mfma -mbmi -mbmi2 -maes -mf16c -mfsgsbase -mlzcnt -mmovbe -mxsave")
endif()

# Option to select build type (Release or Debug)
set(BUILD_TYPE "Release" CACHE STRING "Build type (Release or Debug or Packaging or Regression)")
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to '${BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
endif()

if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Packaging")
  find_package(GTest REQUIRED)
  find_package(OpenCV REQUIRED)
endif()

if(DEFINED DISABLE_DAEMON)
    add_compile_definitions(DISABLE_DAEMON=1)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  set(
    CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -g -fsanitize=address -fsanitize-recover=all"
  )
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -g -fsanitize=address -fsanitize-recover=all"
  )
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)|(x86_64)|(X86_64)")

    if(DEFINED DISABLE_DAEMON)
    set(
      CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -Wall -pipe -fPIC -O3 -ffast-math ${CMAKE_X86_FLAGS_BASE} ${CMAKE_X86_FLAGS_AVX2} -fopenmp"
    )
    set(
      CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -Wall -pipe -fPIC -O3 -ffast-math ${CMAKE_X86_FLAGS_BASE} ${CMAKE_X86_FLAGS_AVX2} -fopenmp"
    )
  else()
    set(
      CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -fvisibility=hidden -Wall -Wextra -pipe -fPIC -static-libgcc -static-libstdc++ -O3 -ffast-math ${CMAKE_X86_FLAGS_BASE} ${CMAKE_X86_FLAGS_AVX2} -fopenmp -Wl,-gc-sections -Wl,--exclude-libs,ALL"
    )
    set(
      CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -fvisibility=hidden -Wall -Wextra -pipe -fPIC -static-libgcc -static-libstdc++ -O3 -ffast-math ${CMAKE_X86_FLAGS_BASE} ${CMAKE_X86_FLAGS_AVX2} -fopenmp -Wl,-gc-sections -Wl,--exclude-libs,ALL"
    )
  endif()

elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64)|(AARCH64)|(arm64)|(ARM64)")

    if(DEFINED DISABLE_DAEMON)
    set(
      CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -Wall -pipe -fPIC -O3 -ffast-math -march=armv8-a+simd -mtune='cortex-a76.cortex-a55' -fopenmp"
    )
    set(
      CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -Wall -pipe -fPIC -O3 -ffast-math -march=armv8-a+simd -mtune='cortex-a76.cortex-a55' -fopenmp"
    )
  else()
    set(
      CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -fvisibility=hidden -Wall -Wextra -pipe -fPIC -static-libgcc -static-libstdc++ -O3 -ffast-math -march=armv8-a+simd -mtune='cortex-a76.cortex-a55' -fopenmp -Wl,-gc-sections -Wl,--exclude-libs,ALL"
    )
    set(
      CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -fvisibility=hidden -Wall -Wextra -pipe -fPIC -static-libgcc -static-libstdc++ -O3 -ffast-math -march=armv8-a+simd -mtune='cortex-a76.cortex-a55' -fopenmp -Wl,-gc-sections -Wl,--exclude-libs,ALL"
    )
  endif()
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "(riscv64)|(RISCV64)|(RV64)|(rv64gc)")
  set(
    CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -Wall -Wextra -pipe -fPIC -O3 -ffast-math -march=rv64gc_zba_zbb_zbs -fopenmp -Wl,-gc-sections -Wl,--exclude-libs,ALL"
  )
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Wextra -pipe -fPIC -O3 -ffast-math -march=rv64gc_zba_zbb_zbs -fopenmp -Wl,-gc-sections -Wl,--exclude-libs,ALL"
  )
endif()

if(NOT DEFINED DISABLE_DAEMON)
  include(grpc_common.cmake)

  # Proto file
  get_filename_component(mx_proto "mx_proc.proto" ABSOLUTE)
  get_filename_component(mx_proto_path "${mx_proto}" PATH)

  # Generated sources
  set(mx_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/mx_proc.pb.cc")
  set(mx_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/mx_proc.pb.h")
  set(mx_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/mx_proc.grpc.pb.cc")
  set(mx_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/mx_proc.grpc.pb.h")
  add_custom_command(
        OUTPUT "${mx_proto_srcs}" "${mx_proto_hdrs}" "${mx_grpc_srcs}" "${mx_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
          --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
          -I "${mx_proto_path}"
          --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
          "${mx_proto}"
        DEPENDS "${mx_proto}")

  # Include generated *.pb.h files
  include_directories("${CMAKE_CURRENT_BINARY_DIR}")

  # mx_grpc_proto
  add_library(mx_grpc_proto
    ${mx_grpc_srcs}
    ${mx_grpc_hdrs}
    ${mx_proto_srcs}
    ${mx_proto_hdrs})
  target_link_libraries(mx_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

  set(MX_GRPC_LIB "mx_grpc_proto")

endif()

#define SUBDIR macro
MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()


enable_testing()
set(MXACCL_DYNAMIC "mx_accl")

SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR} "*")

FOREACH(subdir ${SUBDIRS})
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt)
    ADD_SUBDIRECTORY(${subdir})
  endif()
ENDFOREACH()
