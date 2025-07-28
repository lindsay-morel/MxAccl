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

