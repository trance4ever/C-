cmake_minimum_required(VERSION 2.8)
project(sylar)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)

set(LIB_SRC 
		sylar/address.cc
    sylar/fiber.cc
		sylar/log.cc
		sylar/util.cc
		sylar/config.cc
		sylar/hook.cc
		sylar/bytearray.cc
		sylar/tcp_server.cc
		sylar/thread.cc
		sylar/http/http.cc
		sylar/http/http_parser.cc
		sylar/http/http_session.cc
		sylar/http/http_connection.cc
		sylar/http/servlet.cc
		sylar/http/http_server.cc
		sylar/http/httpclient_parser.rl.cc
		sylar/http/http11_parser.rl.cc
		sylar/timer.cc
		sylar/stream.cc
		sylar/socket_stream.cc
		sylar/scheduler.cc
		sylar/iomanager.cc
		sylar/fd_manager.cc
		sylar/socket.cc
		)

ragelmaker(sylar/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar)

add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) #__FILE__
#add_library(sylar_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar")

set(LIB_LIB
		sylar
		dl
		pthread
		)

#add_executable(test_uri tests/test_uri.cc)
#add_dependencies(test_uri sylar)
#force_redefine_file_macro_for_sources(test_uri) #__FILE__
#target_link_libraries(test_uri ${LIB_LIB} yaml-cpp)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

