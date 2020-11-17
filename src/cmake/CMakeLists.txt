# NOTE: This helper function assumes no generator expressions are used for the source files
# source: https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/
function(target_sources_local target)
  if(POLICY CMP0076)
    # New behavior is available, so just forward to it by ensuring
    # that we have the policy set to request the new behavior, but
    # don't change the policy setting for the calling scope
    cmake_policy(PUSH)
    cmake_policy(SET CMP0076 NEW)
    target_sources(${target} ${ARGN})
    cmake_policy(POP)
    return()
  endif()

  # Must be using CMake 3.12 or earlier, so simulate the new behavior
  unset(_srcList)
  get_target_property(_targetSourceDir ${target} SOURCE_DIR)

  foreach(src ${ARGN})
    if(NOT src STREQUAL "PRIVATE" AND
       NOT src STREQUAL "PUBLIC" AND
       NOT src STREQUAL "INTERFACE" AND
       NOT IS_ABSOLUTE "${src}")
      # Relative path to source, prepend relative to where target was defined
      file(RELATIVE_PATH src "${_targetSourceDir}" "${CMAKE_CURRENT_LIST_DIR}/${src}")
    endif()
    list(APPEND _srcList ${src})
  endforeach()
  target_sources(${target} ${_srcList})
endfunction()


function(add_compile_flags_target target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -march=native)
        target_compile_options(${target} PRIVATE -ffast-math)

        #target_compile_options(${target} PRIVATE -flto)
        #target_link_options(${target} PRIVATE -flto)
        target_link_options(${target} PRIVATE -ltbb)

        # target_compile_options(${target} PRIVATE -funsafe-math-optimizations)

        target_compile_options(${target} PRIVATE -fdiagnostics-color)
        target_compile_options(${target} PRIVATE -Wall)
        target_compile_options(${target} PRIVATE -Wextra)
        target_compile_options(${target} PRIVATE -Werror)

        # only for temporary builds to have a look at the logs
        #target_compile_options(${target} PRIVATE -Weverything)
        #target_compile_options(${target} PRIVATE -Wno-c++98-compat)
        #target_compile_options(${target} PRIVATE -Wno-c++98-compat-pedantic)
        #target_compile_options(${target} PRIVATE -Wno-documentation-unknown-command)
        #target_compile_options(${target} PRIVATE -Wno-undefined-func-template)
        #target_compile_options(${target} PRIVATE -Wno-padded)
        #target_compile_options(${target} PRIVATE -Wno-global-constructors)
        #target_compile_options(${target} PRIVATE -Wno-signed-enum-bitfield)
        #target_compile_options(${target} PRIVATE -Wno-ctad-maybe-unsupported)

        
        

        # for ClangBuildAnalyzer. ccache doesn't work with this.
        # target_compile_options(${target} PRIVATE -ftime-trace)

        target_compile_options(${target} PRIVATE -g)
        
        target_link_options(${target} PRIVATE -lpthread)
    endif()
endfunction()
