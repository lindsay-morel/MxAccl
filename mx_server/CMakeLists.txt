if(NOT DEFINED DISABLE_DAEMON)
  add_executable(mx_server main.cpp)
  target_link_libraries(mx_server memx ${MXACCL_DYNAMIC})
  target_link_libraries(mx_server
              ${MX_GRPC_LIB}
              absl::flags
              absl::flags_parse
              ${_REFLECTION}
              ${_GRPC_GRPCPP}
              ${_PROTOBUF_LIBPROTOBUF})
  target_include_directories(mx_server PRIVATE ${MX_API_DIR}/include/)
  target_compile_options(mx_server PRIVATE -Werror)
  
  if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_link_options(mx_server
      BEFORE PUBLIC PUBLIC -fsanitize=address
    )
  endif()
endif()
