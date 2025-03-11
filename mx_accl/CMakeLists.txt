include_directories("${MX_API_DIR}/include/")
include_directories(${OpenCV_INCLUDE_DIRS})

file(GLOB local_src src/*.cpp src/utils/*.cpp)

add_library(${MXACCL_DYNAMIC} SHARED ${local_src})
target_link_libraries(${MXACCL_DYNAMIC} memx pthread dl uuid)

target_compile_options(${MXACCL_DYNAMIC} PRIVATE -Werror)

if(DEFINED DISABLE_DAEMON)
  target_link_libraries(${MXACCL_DYNAMIC})
else()
  target_link_libraries(${MXACCL_DYNAMIC}
            ${MX_GRPC_LIB}
            absl::flags
            absl::flags_parse
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})
endif()


if(CMAKE_BUILD_TYPE MATCHES "Debug")
  target_link_options(${MXACCL_DYNAMIC}
    BEFORE PUBLIC PUBLIC -fsanitize=address
  )
endif()

if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Packaging")

  add_executable(mxaccl_dfp_tests tests/mxaccl_dfp_tests.cpp )
  target_link_libraries(mxaccl_dfp_tests ${MXACCL_DYNAMIC} gtest gtest_main)
  add_test(NAME mxaccl_dfp_tests COMMAND mxaccl_dfp_tests)

  add_executable(mxaccl_user_tests tests/mxaccl_user_tests.cpp )
  target_link_libraries(mxaccl_user_tests ${MXACCL_DYNAMIC} gtest gtest_main)
  add_test(NAME mxaccl_user_tests COMMAND mxaccl_user_tests)

  add_executable(mxaccl_dataflow_tests tests/mxaccl_dataflow_tests.cpp )
  target_link_libraries(mxaccl_dataflow_tests ${MXACCL_DYNAMIC} gtest gtest_main)
  add_test(NAME mxaccl_dataflow_tests COMMAND mxaccl_dataflow_tests)

  add_executable(mxaccl_accuracy_tests tests/mxaccl_accuracy_tests.cpp)
  target_link_libraries(mxaccl_accuracy_tests ${MXACCL_DYNAMIC} gtest gtest_main ${OpenCV_LIBS})
  add_test(NAME mxaccl_accuracy_tests COMMAND mxaccl_accuracy_tests)

  add_executable(mxaccl_manual_threading_accuracy_tests tests/mxaccl_manual_threading_accuracy.cpp)
  target_link_libraries(mxaccl_manual_threading_accuracy_tests ${MXACCL_DYNAMIC} gtest gtest_main ${OpenCV_LIBS})
  add_test(NAME mxaccl_manual_threading_accuracy_tests COMMAND mxaccl_manual_threading_accuracy_tests)

  add_executable(mxaccl_prepost_tests tests/mxaccl_prepost_tests.cpp )
  target_link_libraries(mxaccl_prepost_tests ${MXACCL_DYNAMIC} gtest gtest_main dl)
  add_test(NAME mxaccl_prepost_tests COMMAND mxaccl_prepost_tests)

  add_executable(mxaccl_prepost_manual_tests tests/mxaccl_prepost_manual_tests.cpp )
  target_link_libraries(mxaccl_prepost_manual_tests ${MXACCL_DYNAMIC} gtest gtest_main dl)
  add_test(NAME mxaccl_prepost_manual_tests COMMAND mxaccl_prepost_manual_tests)

  add_executable(mxaccl_utility_tests tests/mxaccl_utility_tests.cpp )
  target_link_libraries(mxaccl_utility_tests ${MXACCL_DYNAMIC} gtest gtest_main dl)
  add_test(NAME mxaccl_utility_tests COMMAND mxaccl_utility_tests)

  add_executable(mxaccl_multiprocess_connect tests/mxaccl_multiprocess_connect.cpp )
  target_link_libraries(mxaccl_multiprocess_connect ${MXACCL_DYNAMIC} gtest gtest_main dl)
  add_test(NAME mxaccl_multiprocess_connect COMMAND mxaccl_multiprocess_connect)

  add_executable(mxaccl_multiprocess_flow tests/mxaccl_multiprocess_flow.cpp )
  target_link_libraries(mxaccl_multiprocess_flow ${MXACCL_DYNAMIC} gtest gtest_main dl)
  add_test(NAME mxaccl_multiprocess_flow COMMAND mxaccl_multiprocess_flow)

  add_executable(mxaccl_manual_multiprocess_connect tests/mxaccl_manual_multiprocess_connect.cpp )
  target_link_libraries(mxaccl_manual_multiprocess_connect ${MXACCL_DYNAMIC} gtest gtest_main dl)
  add_test(NAME mxaccl_manual_multiprocess_connect COMMAND mxaccl_manual_multiprocess_connect)

  add_executable(mxaccl_manual_multiprocess_flow tests/mxaccl_manual_multiprocess_flow.cpp )
  target_link_libraries(mxaccl_manual_multiprocess_flow ${MXACCL_DYNAMIC} gtest gtest_main dl)
  add_test(NAME mxaccl_manual_multiprocess_flow COMMAND mxaccl_manual_multiprocess_flow)
endif()
