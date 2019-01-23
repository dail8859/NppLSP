CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include(ExternalProject)
include(Hardening)
include(VersionFromGit)
PROJECT(NppLSP)

set(CMAKE_USE_RELATIVE_PATHS TRUE)

set(PACKAGE_NAME "NppLSP" CACHE STRING "Program name")
set(PACKAGE_VERSION ${VERSION})
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
getVersionFromGit(NppLSP "v0.1")

set(Source_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(CMake_Misc_Dir "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#making resource file
set(license_file "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
file(READ ${license_file} LICENSE_TEXT)
string(REPLACE "\"" "\"\"" LICENSE_TEXT ${LICENSE_TEXT})
string(REPLACE "\r\n" "\n" LICENSE_TEXT ${LICENSE_TEXT})
string(REPLACE "\n" "\\r\\n" LICENSE_TEXT ${LICENSE_TEXT})

# config.h
set(CONFIG_FILENAME "Version.h")
set(CONFIG_FILE "${Source_dir}/${CONFIG_FILENAME}")
mark_as_advanced(CONFIG_FILE)
set(CONFIG_FILE_PROTO "${CMake_Misc_Dir}/${CONFIG_FILENAME}.in")

message(STATUS "generating config: ${CONFIG_FILE_PROTO} -> ${CONFIG_FILE}")
configure_file(${CONFIG_FILE_PROTO} ${CONFIG_FILE})

set(NLOHMANN_JSON_DIR_NAME "NLOHMANN_JSON")
set(NLOHMANN_JSON_DIR "${CMAKE_BINARY_DIR}/${NLOHMANN_JSON_DIR_NAME}")
set(NLOHMANN_JSON_INCLUDE_DIR "${NLOHMANN_JSON_DIR}/include")

ExternalProject_Add(
	nlohmann-json
	GIT_REPOSITORY "https://github.com/nlohmann/json.git"
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
	SOURCE_DIR ${NLOHMANN_JSON_DIR}
	PATCH_COMMAND ""
	UPDATE_COMMAND ""
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
)
add_custom_command(OUTPUT ${NLOHMANN_JSON_INCLUDE_DIR} DEPENDS nlohmann-json COMMAND "")

file(GLOB_RECURSE SRCFILES "${Source_dir}/*.c" "${Source_dir}/*.cpp")
file(GLOB_RECURSE resource_files "${Source_dir}/*.rc" "${Source_dir}/*.ico")
if(MSVC_IDE)
   file(GLOB_RECURSE HDRFILES "${Source_dir}/*.h")
endif()

add_library(NppLSP SHARED "${SRCFILES}" "${HDRFILES}" "${resource_files}")
target_include_directories(NppLSP PUBLIC ${NLOHMANN_JSON_INCLUDE_DIR})
target_compile_definitions(NppLSP PUBLIC "-D_UNICODE=1" "-DUNICODE=1")

set_property(TARGET NppLSP PROPERTY CXX_STANDARD 20)
set_property(TARGET NppLSP PROPERTY PREFIX "")
harden(NppLSP)

if(MSVC_IDE)
   source_group("res" FILES  ${resource_files})
endif()
