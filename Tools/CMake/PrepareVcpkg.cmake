function(PrepareVcpkg)
  find_program(VCPKG NAMES vcpkg PATHS ${CMAKE_SOURCE_DIR}/vcpkg)
	if (NOT VCPKG)
		find_program(VCPKG NAMES vcpkg)
  endif()
  if (NOT VCPKG)
    if (NOT (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/vcpkg/.git))
      message(STATUS "----------------------Download vcpkg")
      set(CMD git clone https://github.com/microsoft/vcpkg.git ${CMAKE_SOURCE_DIR}/vcpkg)
      execute_process(COMMAND ${CMD} RESULT_VARIABLE RESULT)

      if (NOT ${RESULT} EQUAL 0)
        message(FATAL_ERROR "Could not download vcpkg")
      endif()
    endif()
    if(NOT (EXISTS ${CMAKE_SOURCE_DIR}/vcpkg/vcpkg OR EXISTS ${CMAKE_SOURCE_DIR}/vcpkg/vcpkg.exe))
      message(STATUS "Compiling vcpkg")
      set(FEXT bat)
      if (UNIX)
        set(FEXT sh)
      endif()
      execute_process(COMMAND ${CMAKE_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.${FEXT} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vcpkg RESULT_VARIABLE RESULT)
      if (NOT ${RESULT} EQUAL 0)
        message(FATAL_ERROR "Could not compile vcpkg")
      endif()
      message(STATUS "SETTING TOOLCHAIN FILE")
      set(VCPKG_ROOT ${CMAKE_SOURCE_DIR})
    endif()
	endif()
  message(STATUS "suka blyat")
  get_filename_component(VCPKG_ROOT ${VCPKG} DIRECTORY CACHE)
  set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE STRING "TF" FORCE)
endfunction()

