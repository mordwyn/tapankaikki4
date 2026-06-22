cmake_minimum_required(VERSION 3.15)
project(tapankaikki)
set(PROJECT_VERSION 4.10)

if(NOT MINGW)
	set(CMAKE_CXX_FLAGS "-std=c++11 -O2 ${CMAKE_CXX_FLAGS}")
else()
	# Workaround for http://stackoverflow.com/q/38538924/3425536
	set(CMAKE_CXX_FLAGS "-std=c++11 -O2 ${CMAKE_CXX_FLAGS}")
	# Add -mwindows to the linker flags to avoid opening a console window when the
	# program is started (see https://cygwin.com/ml/cygwin/2007-04/msg00027.html)
	set(CMAKE_EXE_LINKER_FLAGS -mwindows)
endif()

# dependencies ###########################################################################
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_net REQUIRED)
include_directories(
	${SDL2_INCLUDE_DIRS}
	${SDL2_IMAGE_INCLUDE_DIRS}
	${SDL2_MIXER_INCLUDE_DIRS}
	${SDL2_NET_INCLUDE_DIRS})

# On Windows native win32 api is used and therefore GTK+ is not needed
if(NOT WIN32)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
	include_directories(${GTK3_INCLUDE_DIRS})
endif()


# common #################################################################################
file(GLOB COMMON_SOURCES src/common/*.h src/common/*.cpp)
if(WIN32)
	add_library(tk4-common SHARED ${COMMON_SOURCES})
	# Export symbols, default behaviour is import
	target_compile_definitions(tk4-common PUBLIC -DCOMMON_EXPORTS)
else()
	add_library(tk4-common STATIC ${COMMON_SOURCES})
endif()
target_link_libraries(tk4-common PRIVATE SDL2::SDL2 SDL2_image::SDL2_image)


# game ###################################################################################
file(GLOB GAME_SOURCES src/*.h src/*.cpp src/*.rc)
if(WIN32)
	list(REMOVE_ITEM GAME_SOURCES ${CMAKE_SOURCE_DIR}/src/lists_unix.cpp)
else()
	list(REMOVE_ITEM GAME_SOURCES ${CMAKE_SOURCE_DIR}/src/lists_win32.cpp)
endif()
add_executable(tk4 ${GAME_SOURCES})
target_link_libraries(tk4 tk4-common SDL2_mixer::SDL2_mixer SDL2_net::SDL2_net)


# editor #################################################################################
file(GLOB EDITOR_SOURCES src/Editor/*.h src/Editor/*.cpp src/Editor/*.rc)
if(WIN32)
	list(REMOVE_ITEM EDITOR_SOURCES ${CMAKE_SOURCE_DIR}/src/Editor/EditorLevelIO_gtk.cpp)
else()
	list(REMOVE_ITEM EDITOR_SOURCES ${CMAKE_SOURCE_DIR}/src/Editor/EditorLevelIO_win32.cpp)
endif()
add_executable(tk4-editor ${EDITOR_SOURCES})
target_link_libraries(tk4-editor tk4-common ${GTK3_LIBRARIES})


# efpconv ################################################################################
file(GLOB EFPCONV_SOURCES src/efpconv/*.h src/efpconv/*.cpp)
add_executable(efpconv ${EFPCONV_SOURCES})
target_link_libraries(efpconv tk4-common)

# installation configuration #############################################################
if(UNIX)
	include(GNUInstallDirs)
	install(TARGETS tk4 tk4-editor efpconv DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(DIRECTORY data/ DESTINATION ${CMAKE_INSTALL_DATADIR}/tk4)
	add_definitions(-DDATADIR="${CMAKE_INSTALL_FULL_DATADIR}/tk4")
endif()
