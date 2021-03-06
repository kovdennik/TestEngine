include_guard(GLOBAL)
include(${CMAKE_SOURCE_DIR}/skip_packages.cmake)
if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(ColourBold  "${Esc}[1m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(Magenta     "${Esc}[35m")
  set(Cyan        "${Esc}[36m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan    "${Esc}[1;36m")
  set(BoldWhite   "${Esc}[1;37m")
endif()

function(message)
  list(GET ARGV 0 MessageType)
  if(MessageType STREQUAL FATAL_ERROR OR MessageType STREQUAL SEND_ERROR)
    list(REMOVE_AT ARGV 0)
    _message(${MessageType} "${BoldRed}${ARGV}${ColourReset}")
  elseif(MessageType STREQUAL WARNING)
    list(REMOVE_AT ARGV 0)
    _message(${MessageType} "${BoldYellow}${ARGV}${ColourReset}")
  elseif(MessageType STREQUAL AUTHOR_WARNING)
    list(REMOVE_AT ARGV 0)
    _message(${MessageType} "${BoldCyan}${ARGV}${ColourReset}")
  elseif(MessageType STREQUAL STATUS)
    list(REMOVE_AT ARGV 0)
    _message(${MessageType} "${Green}${ARGV}${ColourReset}")
  else()
    _message("${ARGV}")
  endif()
endfunction()

function(add_private_headers)
	file(GLOB_RECURSE INCLUDE_FILES ${BLACKBOX_PRIVATE_INCLUDE_DIRS}/${PROJECT_NAME}/*.hpp ${BLACKBOX_PRIVATE_INCLUDE_DIRS}/${PROJECT_NAME}/*.h)
	file(GLOB_RECURSE INCLUDE_FILES1 ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
	target_sources(${PROJECT_NAME}
		PRIVATE
		${INCLUDE_FILES}
		${INCLUDE_FILES1}
	)
	source_group(
		TREE ${BLACKBOX_PRIVATE_INCLUDE_DIRS}/${PROJECT_NAME}
		PREFIX "Header Files"
		FILES
		${INCLUDE_FILES}
	)
	source_group(
		TREE ${CMAKE_CURRENT_SOURCE_DIR}
		PREFIX "Header Files"
		FILES
		${INCLUDE_FILES1}
	)
endfunction()

function(install_package Package Result)

  #set(ToInstall "${Package}:${CMAKE_GENERATOR_PLATFORM}-${BB_PLATFORM}")
  set(ToInstall "${Package}:${VCPKG_TARGET_TRIPLET}")
  set(CMD "vcpkg")
  set(ARGS " install ${ToInstall}")
  message(STATUS "Trying install package [${ToInstall}]")
  execute_process(
    COMMAND ${VCPKG} install "${ToInstall}"
    RESULT_VARIABLE result
  )
  set(${Result} ${result} PARENT_SCOPE)
endfunction()

macro(FindGit)
	find_package(Git)
	if (NOT GIT_FOUND)
		message(FATAL_ERROR "Git not found")
	else()
		message(STATUS "Git FOUND at ${GIT_EXECUTABLE}")
	endif()
endmacro()

macro(InstallVcpkg)
	find_program(VCPKG NAMES vcpkg PATHS ${CMAKE_SOURCE_DIR}/vcpkg)
	if (NOT VCPKG)
    include(${TOOLS_CMAKE_DIR}/PrepareVcpkg.cmake)
    #PrepareVcpkg()
		find_program(VCPKG NAMES vcpkg)
		if (NOT VCPKG)
			message(FATAL_ERROR "vcpkg not found, var: ${VCPKG}")
		endif()
	else()
		message(STATUS "VCPKG FOUND at [${VCPKG}]")
	endif()
endmacro()

macro(CommonMacrosInit)
  file(WRITE ${CMAKE_SOURCE_DIR}/deps.txt "")
	message(STATUS "INITIALIZE COMMON MACROS")
	if ((DEFINED LINUX OR MINGW) AND (NOT DEFINED VCPKG_INSTALLER))
		include(FindPkgConfig)
	elseif(DEFINED LINUX AND (DEFINED VCPKG_INSTALLER))
		set(BB_PLATFORM linux)
	else()
		set(BB_PLATFORM windows)
	endif()
  #InstallVcpkg()
endmacro()

function(dump_vars)
	message(STATUS ==============)
	message(STATUS "Dumping ${ARGC} variables:")
	foreach(VAR_NAME ${ARGN})
		message(
		STATUS " ${VAR_NAME} == \"${${VAR_NAME}}\"")
	endforeach()
	message(STATUS ==============)
endfunction()

function(add_package_unix_or_cygwin Package)
	find_package(${ARGV})
endfunction()

function(add_package_msvc Package)
  if (NOT DEFINED VCPKG_INSTALLER)
    find_package(${Package} REQUIRED)
  else (NOT ${Package}_FOUND)
		set(result OFF)
		if (NOT ${Package}_INSTALLED)
			message(STATUS "${Package} yet not installed")
			install_package("${Package}" result)
			if (NOT ${result} EQUAL 0)
				message(FATAL_ERROR "could not install ${Package}") 
			else()
				set(${Package}_INSTALLED ON CACHE STRING "Installed package" FORCE)
			endif()
		endif()
    message(STATUS "FIND CONFIG PACKAGE: ${Package}")
		find_package(${Package} CONFIG)
		if (NOT ${Package}_FOUND)
      message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
			find_package(${Package} REQUIRED)
		endif()
	endif()
endfunction()

function(add_package Package)
  file(APPEND ${CMAKE_SOURCE_DIR}/deps.txt ${Package} "\n")
  set(Skip "-1")
  list(FIND SKIPPED_PACKAGES ${Package} Skip)
  message(STATUS "Package  status ${Sip}")

	find_package(${ARGV})
  if (NOT (${Skip} STREQUAL "-1"))
    message(STATUS "Package ${Package} skipped")
    return()
  else()
    message(STATUS "Package ${Package} not skipped")
  endif()
  if (0 AND (MINGW OR UNIX OR CYGWIN))
    add_package_unix_or_cygwin(${Package})
  else()
    add_package_msvc(${Package})
  endif()
endfunction()

# Helper macro to set default StartUp Project in Visual Studio
macro(set_solution_startup_target target)
	if ("${CMAKE_GENERATOR}" MATCHES "^Visual Studio")
		# Set startup project to launch Game.exe with this project
		set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT ${target})
	endif()
endmacro()

MACRO(SET_PLATFORM_TARGET_PROPERTIES THIS_PROJECT)
	target_compile_definitions( ${THIS_PROJECT} PRIVATE "-DCODE_BASE_FOLDER=\"${CRYENGINE_DIR}/Code/\"")
	target_link_libraries( ${THIS_PROJECT} PRIVATE ${COMMON_LIBS} )

	if(OPTION_DEVELOPER_CONSOLE_IN_RELEASE)
		target_compile_definitions( ${THIS_PROJECT} PRIVATE "-DENABLE_DEVELOPER_CONSOLE_IN_RELEASE")
	endif()

	if (OPTION_REMOTE_CONSOLE)
		target_compile_definitions( ${THIS_PROJECT} PRIVATE "-DUSE_REMOTE_CONSOLE")
	endif()
ENDMACRO()
macro(apply_compile_settings THIS_PROJECT)
	SET_PLATFORM_TARGET_PROPERTIES( ${THIS_PROJECT} )	
endmacro()
macro(add_subsystem subsystem)
	add_subdirectory(${ENGINE_DIR}/${subsystem} ${subsystem})
	#target_link_libraries(${BLACKBOX_PROJECT} INTERFACE ${subsystem})
	set_target_properties(${subsystem} PROPERTIES FOLDER "Engine")

	get_target_property(SOURCE_FILES ${subsystem} SOURCES)
	apply_compile_settings(${subsystem})

	list(APPEND ALL_PROJECT_SOURCES ${SOURCE_FILES})
	list(APPEND ALL_PROJECT_SYSTEMS ${subsystem})
	install_this(${subsystem})
endmacro()

macro(export_subsystem subsystem)

endmacro()

macro(install_this THIS_PROJECT)
	###################################################
	install (
	  TARGETS  ${THIS_PROJECT}
	  EXPORT ${THIS_PROJECT}Config
	  #RUNTIME DESTINATION bin
	  #LIBRARY DESTINATION lib
	  #ARCHIVE DESTINATION lib/static
	)

	# This makes the project importable from the install directory
	# Put config file in per-project dir (name MUST match), can also
	# just go into 'cmake'.
	#install(EXPORT ${THIS_PROJECT}Config DESTINATION .)

	# This makes the project importable from the build directory
	export(TARGETS ${THIS_PROJECT} FILE ${THIS_PROJECT}Config.cmake)
endmacro()

#[[
  function(target_precompile_headers _target)
	if (${CMAKE_VERSION} VERSION_LESS "3.16.0")
		cmake_parse_arguments(
		  _ARGS
		  ""
		  "NOTUSED"
		  "INTERFACE;PRIVATE;PUBLIC"
		  ${ARGN}
		  )
		message("precomiled headers for [${_target}]")
		foreach(interface ${_ARGS_INTERFACE})
		  message(STATUS "interface pch ${interface}")
		endforeach()
		foreach(public ${_ARGS_PUBLIC})
		  message(STATUS "public pch ${public}")
		endforeach()
		foreach(private ${_ARGS_PRIVATE})
		  message(STATUS "private pch ${private}")
		endforeach()
	else()
		_target_precompile_headers(${ARGV})
	endif()

  endfunction()
  ]]

function(add_sdl)
  if (DEFINED LINUX OR DEFINED MINGW)
    if (NOT DEFINED VCPKG_INSTALLER)
      pkg_check_modules(SDL2 SDL2)
      target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
      target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
      message(STATUS "SDL2 include ${SDL2_INCLUDE_DIRS}")

      #[[
      find_package(SDL2 CONFIG REQUIRED)
      target_link_libraries(${PROJECT_NAME} PRIVATE SDL2)

      target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARY})
      target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIR})
      ]]

    else()
      add_package(SDL2)
      target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2-static)
    endif()
	else()
    add_package(SDL2)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)
	endif()
endfunction()

function(add_SDL_net)
	if (DEFINED LINUX OR DEFINED MINGW)
		if (NOT DEFINED VCPKG_INSTALLER)
			pkg_check_modules(SDL2_NET SDL2_net)
			target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_NET_LIBRARIES})
			target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_NET_INCLUDE_DIRS})
			message(STATUS "SDL2 net include ${SDL2_NET_INCLUDE_DIRS}")
		else()
                        add_package(SDL2-net)
                        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2_net)
		endif()
	else()
		add_package(SDL2-net)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2_net)
	endif()
endfunction()

function(Launcher target)
	prepare_project(${ARGN})
	if(ANDROID)
		add_library(${target} SHARED ${${THIS_PROJECT}_SOURCES})
		target_link_libraries(${THIS_PROJECT} PRIVATE m log c android)
		configure_android_launcher(${target})		
	elseif(WINDOWS)
		add_executable(${THIS_PROJECT} WIN32 ${${THIS_PROJECT}_SOURCES})
	else()
		add_executable(${target} ${${THIS_PROJECT}_SOURCES})
	endif()
	if(ORBIS)
		set_property(TARGET ${target} PROPERTY OUTPUT_NAME "${OPTION_LAUNCHER_EXECUTABLE}.elf")	
	elseif(NOT ANDROID)
		set_property(TARGET ${THIS_PROJECT} PROPERTY OUTPUT_NAME "${OPTION_LAUNCHER_EXECUTABLE}")	
	endif()
	if (DURANGO)
		# Set empty so the output directory will default to the solution directory
		set_property(TARGET ${target} PROPERTY ARCHIVE_OUTPUT_DIRECTORY "")	
	endif()
	if(OPTION_STATIC_LINKING)
		use_scaleform()
		target_compile_definitions(${THIS_PROJECT} PRIVATE _LIB -DCRY_IS_MONOLITHIC_BUILD)
		if (WINDOWS)
			set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS_PROFILE " /NODEFAULTLIB:libcpmt.lib")
			set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE " /NODEFAULTLIB:libcpmt.lib")
		endif()
		if (OPTION_STATIC_LINKING_WITH_GAME_AS_DLL)
			set(MODULES_LIST ${MODULES})
		else()
			set(MODULES_LIST ${GAME_MODULES} ${MODULES})
		endif()
		wrap_whole_archive(${target} WRAPPED_MODULES MODULES_LIST)
		target_link_libraries(${THIS_PROJECT} PRIVATE ${WRAPPED_MODULES})
	endif()
	add_metadata(WindowsIcon.ico)
	apply_compile_settings()	

	if(NOT ANDROID)
		if (NOT DEFINED PROJECT_BUILD_CRYENGINE OR PROJECT_BUILD_CRYENGINE)
			install(TARGETS ${target} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
		endif()
	endif()
endfunction()

CommonMacrosInit()
