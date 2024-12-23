# Copyright 2018 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# cmake build file for C++ route_guide example.
# Assumes protobuf and gRPC have been installed using cmake.
# See cmake_externalproject/CMakeLists.txt for all-in-one cmake build
# that automatically builds all the dependencies before building route_guide.

cmake_minimum_required(VERSION 3.8)

if(MSVC)
  add_definitions(-D_WIN32_WINNT=0x600)
endif()

find_package(Threads REQUIRED)
  # Find Protobuf installation
  # Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
option(protobuf_MODULE_COMPATIBLE TRUE)
find_package(protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
