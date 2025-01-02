cmake_minimum_required(VERSION 3.10)

project(quakespasm-exp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

set(COMMON_SOURCES
	Quake/anorm_dots.h
	Quake/anorms.h
	Quake/arch_def.h
	Quake/bgmusic.c
	Quake/bgmusic.h
	Quake/bspfile.h
	Quake/cd_null.c
	Quake/cdaudio.h
	Quake/cfgfile.c
	Quake/cfgfile.h
	Quake/chase.c
	Quake/cl_demo.c
	Quake/cl_input.c
	Quake/cl_main.c
	Quake/cl_parse.c
	Quake/cl_tent.c
	Quake/client.h
	Quake/cmd.c
	Quake/cmd.h
	Quake/common.c
	Quake/common.h
	Quake/console.c
	Quake/console.h
	Quake/crc.c
	Quake/crc.h
	Quake/cvar.c
	Quake/cvar.h
	Quake/draw.h
	Quake/entfixes.cpp
	Quake/filenames.h
	Quake/gl_draw.c
	Quake/gl_fog.c
	Quake/gl_mesh.c
	Quake/gl_model.c
	Quake/gl_model.h
	Quake/gl_refrag.c
	Quake/gl_rlight.c
	Quake/gl_rmain.c
	Quake/gl_rmisc.c
	Quake/gl_screen.c
	Quake/gl_sky.c
	Quake/gl_texmgr.c
	Quake/gl_texmgr.h
	Quake/gl_vidsdl.c
	Quake/gl_warp.c
	Quake/gl_warp_sin.h
	Quake/glquake.h
	Quake/host.c
	Quake/host_cmd.c
	Quake/expmode.cpp
	Quake/image.c
	Quake/image.h
	Quake/in_sdl.c
	Quake/input.h
	Quake/keys.c
	Quake/keys.h
	Quake/lodepng.h
	Quake/ls_common.h
	Quake/ls_edict.cpp
	Quake/ls_engine.cpp
	Quake/ls_imgui.cpp
	Quake/ls_imgui.h
	Quake/ls_imgui_enums.h
	Quake/ls_imgui_funcs.h
	Quake/ls_main.cpp
	Quake/ls_progs.cpp
	Quake/ls_progs_builtins.h
	Quake/ls_vector.cpp
	Quake/ls_vector.h
	Quake/main_sdl.c
	Quake/mathlib.c
	Quake/mathlib.h
	Quake/menu.c
	Quake/menu.h
	Quake/modelgen.h
	Quake/net.h
	Quake/net_defs.h
	Quake/net_dgrm.c
	Quake/net_dgrm.h
	Quake/net_loop.c
	Quake/net_loop.h
	Quake/net_main.c
	Quake/net_sys.h
	Quake/platform.h
	Quake/pr_cmds.c
	Quake/pr_comp.h
	Quake/pr_edict.c
	Quake/pr_exec.c
	Quake/progdefs.h
	Quake/progdefs.q1
	Quake/progs.h
	Quake/protocol.h
	Quake/q_ctype.h
	Quake/q_sound.h
	Quake/q_stdinc.h
	Quake/qs_bmp.h
	Quake/quakedef.h
	Quake/r_alias.c
	Quake/r_brush.c
	Quake/r_part.c
	Quake/r_sprite.c
	Quake/r_world.c
	Quake/render.h
	Quake/resource.h
	Quake/sbar.c
	Quake/sbar.h
	Quake/screen.h
	Quake/server.h
	Quake/snd_codec.c
	Quake/snd_codec.h
	Quake/snd_codeci.h
	Quake/snd_dma.c
	Quake/snd_flac.c
	Quake/snd_flac.h
	Quake/snd_mem.c
	Quake/snd_mikmod.c
	Quake/snd_mikmod.h
	Quake/snd_mix.c
	Quake/snd_mp3.h
	Quake/snd_mp3tag.c
	Quake/snd_opus.c
	Quake/snd_opus.h
	Quake/snd_sdl.c
	Quake/snd_umx.c
	Quake/snd_umx.h
	Quake/snd_vorbis.c
	Quake/snd_vorbis.h
	Quake/snd_wave.c
	Quake/snd_wave.h
	Quake/snd_xmp.c
	Quake/snd_xmp.h
	Quake/spritegn.h
	Quake/stb_image_write.h
	Quake/strl_fn.h
	Quake/strlcat.c
	Quake/strlcpy.c
	Quake/sv_main.c
	Quake/sv_move.c
	Quake/sv_phys.c
	Quake/sv_user.c
	Quake/sys.h
	Quake/vid.h
	Quake/view.c
	Quake/view.h
	Quake/wad.c
	Quake/wad.h
	Quake/world.c
	Quake/world.h
	Quake/wsaerror.h
	Quake/zone.c
	Quake/zone.h
	Quake/default_cfg.h
	Quake/miniz.c
	Quake/miniz.h
)

set(TLSF_SOURCES
	Quake/tlsf/tlsf.c
	Quake/tlsf/tlsf.h
)

set(LUA_DIR Quake/lua)

set(LUA_SOURCES
	${LUA_DIR}/lapi.c
	${LUA_DIR}/lapi.h
	${LUA_DIR}/lauxlib.c
	${LUA_DIR}/lauxlib.h
	${LUA_DIR}/lbaselib.c
	${LUA_DIR}/lcode.c
	${LUA_DIR}/lcode.h
	${LUA_DIR}/lctype.c
	${LUA_DIR}/lctype.h
	${LUA_DIR}/ldebug.c
	${LUA_DIR}/ldebug.h
	${LUA_DIR}/ldo.c
	${LUA_DIR}/ldo.h
	${LUA_DIR}/ldump.c
	${LUA_DIR}/lfunc.c
	${LUA_DIR}/lfunc.h
	${LUA_DIR}/lgc.c
	${LUA_DIR}/lgc.h
	${LUA_DIR}/ljumptab.h
	${LUA_DIR}/llex.c
	${LUA_DIR}/llex.h
	${LUA_DIR}/llimits.h
	${LUA_DIR}/lmathlib.c
	${LUA_DIR}/lmem.c
	${LUA_DIR}/lmem.h
	${LUA_DIR}/lobject.c
	${LUA_DIR}/lobject.h
	${LUA_DIR}/lopcodes.c
	${LUA_DIR}/lopcodes.h
	${LUA_DIR}/loslib.c
	${LUA_DIR}/lparser.c
	${LUA_DIR}/lparser.h
	${LUA_DIR}/lprefix.h
	${LUA_DIR}/lstate.c
	${LUA_DIR}/lstate.h
	${LUA_DIR}/lstring.c
	${LUA_DIR}/lstring.h
	${LUA_DIR}/lstrlib.c
	${LUA_DIR}/ltable.c
	${LUA_DIR}/ltable.h
	${LUA_DIR}/ltablib.c
	${LUA_DIR}/ltm.c
	${LUA_DIR}/ltm.h
	${LUA_DIR}/lua.h
	${LUA_DIR}/lua.hpp
	${LUA_DIR}/luaconf.h
	${LUA_DIR}/lualib.h
	${LUA_DIR}/lundump.c
	${LUA_DIR}/lundump.h
	${LUA_DIR}/lvm.c
	${LUA_DIR}/lvm.h
	${LUA_DIR}/lzio.c
	${LUA_DIR}/lzio.h
)

set(IMGUI_DIR Quake/imgui)
set(IMGUI_BACKENDS_DIR ${IMGUI_DIR}/backends)

set(IMGUI_SOURCES
	${IMGUI_BACKENDS_DIR}/imgui_impl_opengl2.cpp
	${IMGUI_BACKENDS_DIR}/imgui_impl_opengl2.h
	${IMGUI_BACKENDS_DIR}/imgui_impl_sdl2.cpp
	${IMGUI_BACKENDS_DIR}/imgui_impl_sdl2.h
	${IMGUI_DIR}/imconfig.h
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui.h
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_internal.h
	${IMGUI_DIR}/imstb_rectpack.h
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imstb_textedit.h
	${IMGUI_DIR}/imstb_truetype.h
	${IMGUI_DIR}/imgui_widgets.cpp
)

set(HELLO_IMGUI_DIR Quake/hello_imgui)
set(HELLO_IMGUI_SOURCES
	${HELLO_IMGUI_DIR}/impl/imgui_theme.cpp
	${HELLO_IMGUI_DIR}/imgui_theme.h
)

set(IMGUI_COLOR_TEXT_EDIT_DIR Quake/ImGuiColorTextEdit)
set(IMGUI_COLOR_TEXT_EDIT_SOURCES
	${IMGUI_COLOR_TEXT_EDIT_DIR}/LanguageDefinitions.cpp
	${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.cpp
	${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.h
)

set(POSIX_SOURCES
	Quake/net_bsd.c
	Quake/net_udp.c
	Quake/net_udp.h
	Quake/sys_sdl_unix.c
)

set(LINUX_SOURCES
	Quake/pl_linux.c
)

set(MACOS_SOURCES
	Quake/pl_osx.m
)

set(MACOS_BUNDLE_SOURCES
	MacOSX/AppController.h
	MacOSX/AppController.m
	MacOSX/QuakeArgument.h
	MacOSX/QuakeArgument.m
	MacOSX/QuakeArguments.h
	MacOSX/QuakeArguments.m
	MacOSX/ScreenInfo.h
	MacOSX/ScreenInfo.m
	MacOSX/SDLApplication.h
	MacOSX/SDLApplication.m
	MacOSX/SDLMain.h
	MacOSX/SDLMain.m
)

set(MACOS_RESOURCES
	MacOSX/English.lproj/InfoPlist.strings
	MacOSX/English.lproj/Launcher.nib
	MacOSX/Launcher-Info.plist
	MacOSX/QuakeSpasm.icns
)

set(WINDOWS_SOURCES
	Quake/net_win.c
	Quake/net_wins.c
	Quake/net_wins.h
	Quake/net_wipx.c
	Quake/net_wipx.h
	Quake/pl_win.c
	Quake/sys_sdl_win.c
	Windows/QuakeSpasm.ico
	Windows/QuakeSpasm.rc
)

set(OTHER_FILES
	Quake/cd_sdl.c
	Quake/lodepng.c
)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

if(APPLE)
	option(QUAKE_MACOS_BUNDLE "Create macOS application bundle" ON)
	option(QUAKE_MACOS_MOUSE_ACCELERATION "Enable mouse acceleration on macOS" OFF)
endif()

# Set policy needed for CheckIPOSupported module
cmake_policy(SET CMP0069 NEW)

if(QUAKE_MACOS_BUNDLE)
	add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE)
else()
	add_executable(${PROJECT_NAME} WIN32)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
	$<$<CONFIG:Release>:IMGUI_DISABLE_DEBUG_TOOLS>
	$<$<CONFIG:Release>:IMGUI_DISABLE_DEMO_WINDOWS>
	IMGUI_DISABLE_OBSOLETE_FUNCTIONS
	IMGUI_EDITOR_NO_BOOST
	IMGUI_EDITOR_QSEXP
	USE_CODEC_UMX
	USE_CODEC_WAVE
	# USE_HELLO_IMGUI
	USE_IMGUI
	USE_LUA_SCRIPTING
	USE_TLSF
)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	target_compile_options(${PROJECT_NAME} PRIVATE /MP /wd4996)

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		# Disable the following warnings, Lua files only
		# - warning C4244: 'function': conversion from '__int64' to 'lua_Integer', possible loss of data
		# - warning C4267: 'function': conversion from 'size_t' to 'lua_Integer', possible loss of data
		set_source_files_properties(${LUA_SOURCES} PROPERTIES COMPILE_FLAGS "/wd4244 /wd4267")
	endif()

	set_source_files_properties(${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.cpp
		PROPERTIES COMPILE_FLAGS "/wd4018 /wd4244 /wd4267")

	# Disable C++ exceptions and RTTI
	# Altering of global CMAKE_CXX_FLAGS is required because the following line
	#   target_compile_options(${PROJECT_NAME} PRIVATE /EHs-c- /GR-)
	# cause these compilation warnings
	#   warning D9025: overriding '/EHs' with '/EHs-'
	#   warning D9025: overriding '/EHc' with '/EHc-'
	string(REGEX REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REGEX REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	add_definitions(-D_HAS_EXCEPTIONS=0)  # This one is for STL

	# Enable correct definition of __cplusplus macro, it's always set to 199711L otherwise
	# See https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
	# and https://gitlab.kitware.com/cmake/cmake/-/issues/18837
	add_compile_options(/Zc:__cplusplus)
else()
	option(QUAKE_SEARCH_USERDIRS "Search user directories for data" OFF)

	if(QUAKE_SEARCH_USERDIRS)
		target_compile_definitions(${PROJECT_NAME} PRIVATE DO_USERDIRS)
	endif()

	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wno-trigraphs)
	set_source_files_properties(${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.cpp
		PROPERTIES COMPILE_FLAGS "-Wno-unused-variable -Wno-sign-compare")

	# Disable C++ exceptions and RTTI
	# target_compile_options() doesn't fit very well as GCC complains on every C file
	#   warning: command-line option ‘-fno-rtti’ is valid for C++/D/ObjC++ but not for C
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
	Quake
	${LUA_DIR}
	${IMGUI_DIR}
	"${CMAKE_BINARY_DIR}"
)

if(APPLE)
	if(NOT QUAKE_MACOS_MOUSE_ACCELERATION)
		target_compile_definitions(${PROJECT_NAME} PRIVATE MACOS_X_ACCELERATION_HACK)

		# Disable a bunch of IOKit deprecation warnings
		target_compile_options(${PROJECT_NAME} PRIVATE -Wno-deprecated-declarations)
	endif()

	target_compile_definitions(${PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
	target_link_libraries(${PROJECT_NAME} PRIVATE
		"-framework AppKit"
		"-framework IOKit"
	)

	# Disable macro redefinition warning
	set_source_files_properties(${IMGUI_BACKENDS_DIR}/imgui_impl_opengl2.cpp PROPERTIES
		COMPILE_FLAGS -UGL_SILENCE_DEPRECATION
	)

	if(${CMAKE_GENERATOR} STREQUAL Xcode)
		target_compile_options(${PROJECT_NAME} PRIVATE -Wno-shorten-64-to-32)
	endif()

	set(PLATFORM_SOURCE
		${MACOS_SOURCES}
		${POSIX_SOURCES}
	)
	set(OTHER_FILES
		${LINUX_SOURCES}
		${WINDOWS_SOURCES}
		${OTHER_FILES}
	)

	if(QUAKE_MACOS_BUNDLE)
		set(PLATFORM_SOURCE
			${PLATFORM_SOURCE}
			${MACOS_BUNDLE_SOURCES}
			${MACOS_RESOURCES}
		)

		target_include_directories(${PROJECT_NAME} PRIVATE MacOSX)

		set_target_properties(${PROJECT_NAME} PROPERTIES
			MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/MacOSX/Info.plist"
			XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "net.sf.quakespasm.QuakeSpasm"
		)
		set_source_files_properties(${MACOS_RESOURCES} PROPERTIES
			MACOSX_PACKAGE_LOCATION Resources
		)
	else()
		set(PLATFORM_SOURCE
			${PLATFORM_SOURCE}
			Quake/noappbundle_main.c
		)
		set(OTHER_FILES
			${OTHER_FILES}
			${MACOS_BUNDLE_SOURCES}
			${MACOS_RESOURCES}
		)
	endif()
elseif(WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE Winmm Ws2_32)

	set(PLATFORM_SOURCE
		${WINDOWS_SOURCES}
	)
	set(OTHER_FILES
		${LINUX_SOURCES}
		${MACOS_SOURCES}
		${POSIX_SOURCES}
		${OTHER_FILES}
	)
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE m)

	set(PLATFORM_SOURCE
		${LINUX_SOURCES}
		${POSIX_SOURCES}
	)
	set(OTHER_FILES
		${MACOS_SOURCES}
		${WINDOWS_SOURCES}
		${OTHER_FILES}
	)
endif()

option(QUAKE_GENERATE_VERSION_HEADER "Generate version header file during configuration step" OFF)

if(QUAKE_GENERATE_VERSION_HEADER)
	set(VERSION_HEADER "${CMAKE_BINARY_DIR}/expversion.h")

	find_package(Git REQUIRED)
	execute_process(COMMAND
		${GIT_EXECUTABLE} describe --tags
		OUTPUT_STRIP_TRAILING_WHITESPACE
		OUTPUT_VARIABLE QUAKE_VERSION
	)
	file(WRITE ${VERSION_HEADER} "static const char* const expversion = \"${QUAKE_VERSION}\";\n")

	set(COMMON_SOURCES ${COMMON_SOURCES} ${VERSION_HEADER})
endif()

set(ENTFIXES_HEADER "${CMAKE_BINARY_DIR}/entfixes.h")

set(ENGINE_SOURCES
	${COMMON_SOURCES}
	${PLATFORM_SOURCE}
	${OTHER_FILES}
	${ENTFIXES_HEADER}
)

source_group("Engine" FILES ${ENGINE_SOURCES} Quake/snd_mp3.c Quake/snd_mpg123.c)
source_group("Hello ImGui" FILES ${HELLO_IMGUI_SOURCES})
source_group("ImGui" FILES ${IMGUI_SOURCES})
source_group("ImGuiColorTextEdit" FILES ${IMGUI_COLOR_TEXT_EDIT_SOURCES})
source_group("Lua" FILES ${LUA_SOURCES})
source_group("TLSF" FILES ${TLSF_SOURCES})

target_sources(${PROJECT_NAME} PRIVATE
	${ENGINE_SOURCES}
	${HELLO_IMGUI_SOURCES}
	${IMGUI_SOURCES}
	${IMGUI_COLOR_TEXT_EDIT_SOURCES}
	${LUA_SOURCES}
	${TLSF_SOURCES}
)

# TODO: Remove when USE_HELLO_IMGUI is defined
set(OTHER_FILES ${OTHER_FILES} ${HELLO_IMGUI_SOURCES})

set_source_files_properties(Quake/progdefs.q1 PROPERTIES LANGUAGE C HEADER_FILE_ONLY TRUE)
set_source_files_properties(${OTHER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

option(QUAKE_LTO "Enable link-time optimization if supported by compiler" OFF)

if(QUAKE_LTO)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT QUAKE_LTO_SUPPORTED OUTPUT QUAKE_LTO_ERROR)

	if(QUAKE_LTO_SUPPORTED)
		set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
	else()
		message("Link-time optimization was not enabled because of error: ${QUAKE_LTO_ERROR}")
	endif()
endif()

option(QUAKE_WARNINGS_AS_ERRORS "Treat compilation warnings as errors" OFF)

if(QUAKE_WARNINGS_AS_ERRORS)
	if(MSVC)
		target_compile_options(${PROJECT_NAME} PRIVATE /WX)
	else()
		target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
	endif()
endif()

# Required dependencies

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE "${OPENGL_INCLUDE_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE "${OPENGL_LIBRARY}")

include(FindPackageHandleStandardArgs)

find_path(SDL2_INCLUDE_DIR SDL.h PATH_SUFFIXES SDL2)
find_library(SDL2_LIBRARY SDL2)
find_package_handle_standard_args(SDL2 DEFAULT_MSG SDL2_LIBRARY SDL2_INCLUDE_DIR)

if(NOT SDL2_FOUND AND WIN32)
	message(STATUS "Using vendored SDL2")
	set(SDL2_FOUND TRUE)
	set(SDL2_DIR Windows/SDL2)
	set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include")
	set(SDL2_LIBRARY SDL2 SDL2main)

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		target_link_directories(${PROJECT_NAME} PRIVATE "${SDL2_DIR}/lib64")
	else()
		target_link_directories(${PROJECT_NAME} PRIVATE "${SDL2_DIR}/lib")
	endif()
endif()

if(SDL2_FOUND)
	target_compile_definitions(${PROJECT_NAME} PRIVATE USE_SDL2)
	target_include_directories(${PROJECT_NAME} PRIVATE "${SDL2_INCLUDE_DIR}")
	target_link_libraries(${PROJECT_NAME} PRIVATE "${SDL2_LIBRARY}")
else()
	message(SEND_ERROR "Could NOT find SDL2")
endif()

# Optional dependencies

function(FIND_CODEC)
	cmake_parse_arguments(FIND_CODEC "" "NAME;HEADER;DEFINE" "PATH_SUFFIXES;LIBRARIES;SOURCES" ${ARGN})

	set(CODEC ${FIND_CODEC_NAME})
	find_path(${CODEC}_INCLUDE_DIR ${FIND_CODEC_HEADER} PATH_SUFFIXES ${FIND_CODEC_PATH_SUFFIXES})

	foreach(LIB IN LISTS FIND_CODEC_LIBRARIES)
		find_library(${LIB}_PATH ${LIB})
		list(APPEND ${CODEC}_LIBRARY "${${LIB}_PATH}")
	endforeach()

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(${CODEC} DEFAULT_MSG ${CODEC}_LIBRARY ${CODEC}_INCLUDE_DIR)

	if(NOT ${CODEC}_FOUND AND WIN32)
		message(STATUS "Using vendored ${CODEC}")
		set(${CODEC}_FOUND TRUE)
		set(CODECS_DIR Windows/codecs)
		set(${CODEC}_INCLUDE_DIR "${CODECS_DIR}/include")
		set(${CODEC}_LIBRARY "")

		foreach(LIB IN LISTS FIND_CODEC_LIBRARIES)
			list(APPEND ${CODEC}_LIBRARY "lib${LIB}")
		endforeach()

		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			target_link_directories(${PROJECT_NAME} PRIVATE "${CODECS_DIR}/x64")
		else()
			target_link_directories(${PROJECT_NAME} PRIVATE "${CODECS_DIR}/x86")
		endif()
	endif()

	if(${CODEC}_FOUND)
		set(${CODEC}_FOUND TRUE PARENT_SCOPE)
		target_sources(${PROJECT_NAME} PRIVATE "${FIND_CODEC_SOURCES}")
		target_compile_definitions(${PROJECT_NAME} PRIVATE ${FIND_CODEC_DEFINE})
		target_include_directories(${PROJECT_NAME} PRIVATE "${${CODEC}_INCLUDE_DIR}")
		target_link_libraries(${PROJECT_NAME} PRIVATE "${${CODEC}_LIBRARY}")
	endif()
endfunction()

find_codec(NAME FLAC
	HEADER FLAC/stream_decoder.h
	LIBRARIES FLAC
	DEFINE USE_CODEC_FLAC
)

find_codec(NAME MIKMOD
	HEADER mikmod.h
	LIBRARIES mikmod
	DEFINE USE_CODEC_MIKMOD
)

find_codec(NAME MPG123
	HEADER mpg123.h
	LIBRARIES mpg123
	DEFINE USE_CODEC_MP3
	SOURCES Quake/snd_mpg123.c
)

if(NOT MPG123_FOUND)
	find_codec(NAME MAD
		HEADER mad.h
		LIBRARIES mad
		DEFINE USE_CODEC_MP3
		SOURCES Quake/snd_mp3.c
	)
endif()

find_codec(NAME OPUS
	HEADER opusfile.h
	PATH_SUFFIXES opus
	LIBRARIES opus opusfile
	DEFINE USE_CODEC_OPUS
)

find_codec(NAME VORBIS
	HEADER vorbis/vorbisfile.h
	LIBRARIES vorbis vorbisfile
	DEFINE USE_CODEC_VORBIS
)

find_codec(NAME XMP
	HEADER xmp.h
	LIBRARIES xmp
	DEFINE USE_CODEC_XMP
)

if(CMAKE_CROSSCOMPILING)
	find_package(EntFixesGenerator)
else()
	set(GENENTFIXES_DIR Misc/entfixes/generator)
	set(OPENVCDIFF_DIR ${GENENTFIXES_DIR}/open-vcdiff)
	set(OPENVCDIFF_GOOGLE_DIR ${OPENVCDIFF_DIR}/google)
	set(OPENVCDIFF_ZLIB_DIR ${OPENVCDIFF_DIR}/zlib)

	add_executable(entfixesgenerator
		${OPENVCDIFF_GOOGLE_DIR}/codetablewriter_interface.h
		${OPENVCDIFF_GOOGLE_DIR}/encodetable.h
		${OPENVCDIFF_GOOGLE_DIR}/format_extension_flags.h
		${OPENVCDIFF_GOOGLE_DIR}/jsonwriter.h
		${OPENVCDIFF_GOOGLE_DIR}/output_string.h
		${OPENVCDIFF_GOOGLE_DIR}/vcencoder.h
		${OPENVCDIFF_ZLIB_DIR}/adler32.c
		${OPENVCDIFF_ZLIB_DIR}/zconf.h
		${OPENVCDIFF_ZLIB_DIR}/zlib.h
		${OPENVCDIFF_DIR}/addrcache.cc
		${OPENVCDIFF_DIR}/blockhash.cc
		${OPENVCDIFF_DIR}/codetable.cc
		${OPENVCDIFF_DIR}/encodetable.cc
		${OPENVCDIFF_DIR}/instruction_map.cc
		${OPENVCDIFF_DIR}/jsonwriter.cc
		${OPENVCDIFF_DIR}/logging.cc
		${OPENVCDIFF_DIR}/varint_bigendian.cc
		${OPENVCDIFF_DIR}/vcdiffengine.cc
		${OPENVCDIFF_DIR}/vcencoder.cc
		${OPENVCDIFF_DIR}/addrcache.h
		${OPENVCDIFF_DIR}/blockhash.h
		${OPENVCDIFF_DIR}/checksum.h
		${OPENVCDIFF_DIR}/codetable.h
		${OPENVCDIFF_DIR}/compile_assert.h
		${OPENVCDIFF_DIR}/config.h
		${OPENVCDIFF_DIR}/instruction_map.h
		${OPENVCDIFF_DIR}/logging.h
		${OPENVCDIFF_DIR}/rolling_hash.h
		${OPENVCDIFF_DIR}/unique_ptr.h
		${OPENVCDIFF_DIR}/varint_bigendian.h
		${OPENVCDIFF_DIR}/vcdiff_defs.h
		${OPENVCDIFF_DIR}/vcdiffengine.h
		${GENENTFIXES_DIR}/generator.cpp
	)
	set_target_properties(entfixesgenerator PROPERTIES EXCLUDE_FROM_ALL YES)
	target_include_directories(entfixesgenerator PRIVATE ${OPENVCDIFF_ZLIB_DIR} ${OPENVCDIFF_DIR})

	export(TARGETS entfixesgenerator FILE "${CMAKE_BINARY_DIR}/EntFixesGeneratorConfig.cmake")
endif()

add_custom_target(genentfixes
	COMMAND entfixesgenerator "${CMAKE_SOURCE_DIR}/Misc/entfixes/entities" ${ENTFIXES_HEADER}
	BYPRODUCTS ${ENTFIXES_HEADER}
	DEPENDS entfixesgenerator
)
add_dependencies(${PROJECT_NAME} genentfixes)

option(QUAKE_BUILD_ENGINE_PAK "Build engine .pak file from individual files" ON)

set(ENGINE_PAK_FILENAME quakespasm-exp.pak)

if(QUAKE_BUILD_ENGINE_PAK)
	if(CMAKE_CROSSCOMPILING)
		find_package(MakeQuakePak)
	else()
		add_executable(makeqpak Misc/makeqpak.c)
		export(TARGETS makeqpak FILE "${CMAKE_BINARY_DIR}/MakeQuakePakConfig.cmake")
	endif()

	set(ENGINE_PAK_SOURCES
		gfx/conback.lmp
		scripts/aliases/common.cfg
		scripts/aliases/expmode.cfg
		scripts/edicts.lua
		scripts/engine.lua
		scripts/expmode_base.lua
		scripts/expmode_edicts.lua
		scripts/expmode_engine.lua
		scripts/expmode_progs.lua
		scripts/progs.lua
		# debug scripts are excluded intentionally
		default.cfg
	)

	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND makeqpak "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ENGINE_PAK_FILENAME}" ${ENGINE_PAK_SOURCES}
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Misc/qs_pak"
	)
else()
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CMAKE_SOURCE_DIR}/Quake/quakespasm.pak"
			"$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ENGINE_PAK_FILENAME}"
	)
endif()
