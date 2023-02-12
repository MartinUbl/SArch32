CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(StupidArch32)

SET(CMAKE_CXX_STANDARD 20)

FIND_PACKAGE(Qt5 COMPONENTS Core Widgets)

FILE(GLOB_RECURSE emulator_src emulator/*.cpp emulator/*.c emulator/*.h emulator/*.hpp)
FILE(GLOB_RECURSE assembler_src assembler/*.cpp assembler/*.c assembler/*.h assembler/*.hpp)

FILE(GLOB_RECURSE core_src core/*.cpp core/*.h core/*.c core/*.hpp)

ADD_LIBRARY(SArch32_core STATIC ${core_src})

ADD_EXECUTABLE(SArch32_assembler ${assembler_src})

ADD_EXECUTABLE(SArch32_emulator ${emulator_src})

TARGET_INCLUDE_DIRECTORIES(SArch32_emulator PUBLIC ${Qt5_INCLUDE_DIRS})
SET_PROPERTY(TARGET SArch32_emulator PROPERTY AUTOMOC ON)

TARGET_LINK_LIBRARIES(SArch32_emulator SArch32_core Qt5::Core Qt5::Widgets)
TARGET_LINK_LIBRARIES(SArch32_assembler SArch32_core)
