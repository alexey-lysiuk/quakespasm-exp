cmake_minimum_required(VERSION 3.0)

project(quakespasm-exp LANGUAGES C)

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
	Quake/image.c
	Quake/image.h
	Quake/in_sdl.c
	Quake/input.h
	Quake/keys.c
	Quake/keys.h
	Quake/lodepng.h
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
)

set(WINDOWS_SOURCES
	${WINDOWS_SOURCES}
	Windows/QuakeSpasm.ico
	Windows/QuakeSpasm.rc
)

set(OTHER_FILES
	Quake/cd_sdl.c
	Quake/lodepng.c
)

if(NOT DCMAKE_BUILD_TYPE)
	set(DCMAKE_BUILD_TYPE Release)
endif()

if(APPLE)
	option(QUAKE_MACOS_BUNDLE "Create macOS application bundle" ON)
	option(QUAKE_MACOS_MOUSE_ACCELERATION "Enable mouse acceleration on macOS" OFF)
endif()

if(QUAKE_MACOS_BUNDLE)
	add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE)
else()
	add_executable(${PROJECT_NAME} WIN32)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
	USE_CODEC_UMX
	USE_CODEC_WAVE
)

if(MSVC)
	target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
	set(CMAKE_C_FLAGS "/MP /wd4996 ${CMAKE_C_FLAGS}")
else()
	option(QUAKE_SEARCH_USERDIRS "Search user directories for data" OFF)

	if(QUAKE_SEARCH_USERDIRS)
		target_compile_definitions(${PROJECT_NAME} PRIVATE DO_USERDIRS)
	endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE Quake)

if(APPLE)
	if(NOT QUAKE_MACOS_MOUSE_ACCELERATION)
		target_compile_definitions(${PROJECT_NAME} PRIVATE MACOS_X_ACCELERATION_HACK)
	endif()

	target_compile_definitions(${PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
	target_link_libraries(${PROJECT_NAME} PRIVATE
		"-framework AppKit"
		"-framework IOKit"
	)

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
		set(OTHER_FILES
			${OTHER_FILES}
			${MACOS_BUNDLE_SOURCES}
			${MACOS_RESOURCES}
		)

		# Workaround missing main function
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-alias -Wl,_SDL_main -Wl,_main") 
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

target_sources(${PROJECT_NAME} PRIVATE
	${COMMON_SOURCES}
	${PLATFORM_SOURCE}
	${OTHER_FILES}
)

set_source_files_properties(${OTHER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

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

set(ENGINE_PAK_FILE quakespasm.pak)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND
	${CMAKE_COMMAND} -E copy_if_different
	"${CMAKE_SOURCE_DIR}/Quake/${ENGINE_PAK_FILE}"
	"$<TARGET_PROPERTY:${PROJECT_NAME},BINARY_DIR>/$<CONFIG>/${ENGINE_PAK_FILE}"
)