cmake_minimum_required(VERSION 2.8.4)
project(net)

set(CMAKE_CXX_STANDARD  11)

#add_definitions(-DSHINY_IS_COMPILED=FALSE)

include_directories("../")

set(SOURCE_FILES	Address.h
                    Address.cpp
                    IConnection.h
                    Socket.h
					Socket.cpp
                    UDPConnection.h
					UDPConnection.cpp
                    PacketQueue.h
                    ReliabilitySystem.h
					ReliabilitySystem.cpp
                    ReliableUDPConnection.h
					ReliableUDPConnection.cpp
                    Var.h
			        VarContainer.h
					VarContainer.cpp
                    HTTPBase.h
			        HTTPRequest.h
			        HTTPResponse.h
					HTTPBase.cpp
					HTTPRequest.cpp
					HTTPResponse.cpp
                    BitMsg.h
					BitMsg.cpp
                    Types.h GLMTypes.h
                    Compress.h
                    Compress.cpp
                    NAT_PMP.h
                    NAT_PMP.cpp
                    NAT_PCP.h
                    NAT_PCP.cpp
                    WsConnection.h
                    WsConnection.cpp)

list(APPEND ALL_SOURCE_FILES ${SOURCE_FILES})
set_source_files_properties(${SOURCE_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("src" FILES ${SOURCE_FILES})

list(APPEND ALL_SOURCE_FILES _net.cpp)
source_group("src" FILES _net.cpp)

add_library(net STATIC ${ALL_SOURCE_FILES})
#add_library(net STATIC _unitybuild_.cpp)
if(WIN32)
    target_link_libraries(net ws2_32 hash minilzo)
else()
    target_link_libraries(net hash minilzo)
endif()

add_subdirectory(tests)
