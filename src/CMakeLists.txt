cmake_minimum_required (VERSION 3.5)

project(scarab C CXX ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(flags_enable_pt_memtrace "")

if(DEFINED ENV{SCARAB_ENABLE_PT_MEMTRACE})
  set(flags_enable_pt_memtrace "-DENABLE_PT_MEMTRACE")
endif()

set(CMAKE_C_FLAGS_SCARABOPT   "-O3 -g3 -DNO_DEBUG -DLINUX -DX86_64 ${flags_enable_pt_memtrace}")
set(CMAKE_CXX_FLAGS_SCARABOPT "-O3 -g3 -DNO_DEBUG -DLINUX -DX86_64 ${flags_enable_pt_memtrace}")
set(CMAKE_C_FLAGS_VALGRIND    "-O0 -g3 -DLINUX -DX86_64 ${flags_enable_pt_memtrace}")
set(CMAKE_CXX_FLAGS_VALGRIND  "-O0 -g3 -DLINUX -DX86_64 ${flags_enable_pt_memtrace}")
set(CMAKE_C_FLAGS_GPROF       "${CMAKE_CXX_FLAGS_SCARABOPT} -pg -g3 ${flags_enable_pt_memtrace}")
set(CMAKE_CXX_FLAGS_GPROF     "${CMAKE_CXX_FLAGS_SCARABOPT} -pg -g3 ${flags_enable_pt_memtrace}")
set(CMAKE_C_FLAGS_DEBUG       "-O0 -g3 -DLINUX -DX86_64 -fsanitize=address -fsanitize-address-use-after-scope ${flags_enable_pt_memtrace}")
set(CMAKE_CXX_FLAGS_DEBUG     "-O0 -g3 -DLINUX -DX86_64 -fsanitize=address -fsanitize-address-use-after-scope ${flags_enable_pt_memtrace}")

# Turn off doc generation before adding the subdirectory
set(BUILD_DOCS OFF CACHE BOOL "Disable DynamoRIO doc generation" FORCE)
#build dependencies with default warn flags, otherwise dynamorio will not build
add_subdirectory(deps)

set(warn_flags -Wall -Wunused -Wno-long-long
  -Wpointer-arith
  -Werror)
set(warn_c_flags ${warn_flags}
  -Wmissing-declarations
  -Wmissing-prototypes
  -Wimplicit -Wno-unused-but-set-variable -Wno-maybe-uninitialized)
set(warn_cxx_flags ${warn_flags})

add_compile_options(
  "$<$<COMPILE_LANGUAGE:C>:${warn_c_flags}>"
  "$<$<COMPILE_LANGUAGE:CXX>:${warn_cxx_flags}>"
)

add_subdirectory(ramulator)
add_subdirectory(pin/pin_lib)
add_subdirectory(pin/pin_exec/testing)

set(scarab_dirs bp debug bp/template_lib dvfs frontend globals isa libs memory power prefetcher confidence .)
if(DEFINED ENV{SCARAB_ENABLE_PT_MEMTRACE})
  set(scarab_dirs ${scarab_dirs} frontend/pt_memtrace)
endif()

set(srcs)
foreach(dir IN LISTS scarab_dirs) 
    file(GLOB dir_srcs
      ${dir}/*.h
      ${dir}/*.cc
      ${dir}/*.c
    )
    set(srcs ${srcs} ${dir_srcs})
endforeach()

add_executable(scarab 
    ${srcs}
)

target_include_directories(scarab PRIVATE .)

target_link_libraries(scarab
    PRIVATE
        ramulator
        pin_lib_for_scarab
)
if(DEFINED ENV{SCARAB_ENABLE_PT_MEMTRACE})
  target_link_libraries(scarab PRIVATE dynamorio pt_memtrace)
endif()
