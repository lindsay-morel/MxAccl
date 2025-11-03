include_directories("${MX_API_DIR}/include/")

file(GLOB local_src src/*.cpp src/utils/*.cpp)

add_library(${MXACCL_DYNAMIC} SHARED ${local_src})
target_link_libraries(${MXACCL_DYNAMIC} memx pthread dl cpuinfo)
set_target_properties(${MXACCL_DYNAMIC} PROPERTIES SOVERSION 2)

target_compile_options(${MXACCL_DYNAMIC} PRIVATE -Werror)

target_include_directories(${MXACCL_DYNAMIC} PRIVATE ${MX_API_HOME_DIR}/extern/asio)
target_include_directories(${MXACCL_DYNAMIC} PRIVATE ${MX_API_HOME_DIR}/extern/spdlog)

install(TARGETS ${MXACCL_DYNAMIC}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
       )

install(DIRECTORY ${MX_API_DIR}/include/memx/ DESTINATION include/memx)


if(CMAKE_BUILD_TYPE MATCHES "Debug")
  target_link_options(${MXACCL_DYNAMIC}
    BEFORE PUBLIC PUBLIC
  )
endif()

if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Packaging")

  include_directories(${OpenCV_INCLUDE_DIRS})

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
