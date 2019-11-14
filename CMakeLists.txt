cmake_minimum_required(VERSION 3.10.2)
project(flexus)

include(Configuration.cmake)
foreach(SIM ${SIMULATORS})
    include(${FLEXUS_ROOT}/simulators/${SIM}/${SIM}.cmake)
endforeach()
include_directories(${FLEXUS_ROOT})
include_directories(SYSTEM ${BOOST_INCLUDEDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${FLEXUS_ROOT}/CMakeFiles)

# Add option to force colored output
option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." FALSE)

if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

# compile core
file(GLOB_RECURSE CORE_SOURCE ${FLEXUS_ROOT}/core/*.cpp)
file(GLOB_RECURSE QEMU_SOUECE ${FLEXUS_ROOT}/core/qemu/*.cpp)
list(FILTER CORE_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/core/test")
list(FILTER CORE_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/core/qemu")
add_library(core STATIC ${CORE_SOURCE})
add_library(qemu STATIC ${QEMU_SOUECE})
target_compile_options(core PRIVATE -c ${GCC_FLAGS})
target_compile_options(qemu PRIVATE -c ${GCC_FLAGS})

# Get set union of components required by all simulator
set(ALL_COMPONENTS)
foreach(SIM ${SIMULATORS})
  list(APPEND ALL_COMPONENTS ${${SIM}_REQUIRED_COMPONENTS})
endforeach()
list(REMOVE_DUPLICATES ALL_COMPONENTS)

# Compile components
foreach(COMPONENT ${ALL_COMPONENTS})
  file(GLOB_RECURSE COMPONENT_${COMPONENT}_SOURCE "${FLEXUS_ROOT}/components/${COMPONENT}/*.cpp")
  if (${COMPONENT} STREQUAL "NetShim")
    list(FILTER COMPONENT_NetShim_SOURCE EXCLUDE REGEX "^${FLEXUS_ROOT}/components/NetShim/testing")
  endif()
  add_library(${COMPONENT} STATIC ${COMPONENT_${COMPONENT}_SOURCE})
  target_compile_options(${COMPONENT} PRIVATE -c ${GCC_FLAGS})
endforeach()

# compile stat-manager
file(GLOB_RECURSE STATMGR_DEPS ${FLEXUS_ROOT}/core/debug/*.cpp)
set(STATMGR_DEPS ${STATMGR_DEPS} ${FLEXUS_ROOT}/core/stats/stats.cpp
  ${FLEXUS_ROOT}/core/stats/stats_calc.cpp ${FLEXUS_ROOT}/core/stats/measurement.cpp)
#file(GLOB_RECURSE STATMGR_SOURCE ${FLEXUS_ROOT}/stat-manager/*.cpp)
foreach(EXEC stat-manager stat-collapse stat-sample)
  add_executable(${EXEC} ${FLEXUS_ROOT}/stat-manager/${EXEC}.cpp ${STATMGR_DEPS})
  target_compile_options(${EXEC} PRIVATE -c ${GCC_FLAGS})
  target_link_libraries(${EXEC} "-L${BOOST_LIBRARYDIR}"
    boost_system boost_regex boost_serialization boost_iostreams z)
endforeach()

# compile simulators
foreach(SIM ${SIMULATORS})
  add_library(wiring_${SIM} OBJECT ${FLEXUS_ROOT}/simulators/${SIM}/wiring.cpp)
  target_compile_options(wiring_${SIM} PRIVATE -c ${GCC_FLAGS})
  add_library(${SIM} SHARED $<TARGET_OBJECTS:wiring_${SIM}>)
  target_link_libraries(${SIM} "-Wl,--whole-archive" "-Wl,-export-dynamic" ${${SIM}_REQUIRED_COMPONENTS})
  target_link_libraries(${SIM} "-Wl,--whole-archive" core qemu "-Wl,--no-whole-archive")
  target_link_libraries(${SIM} "-L${BOOST_LIBRARYDIR}" boost_system boost_regex boost_serialization boost_iostreams z)
endforeach()

# clean for cmake
add_custom_target(clean_cmake
    COMMAND rm -rf *.o *.a *.so
    COMMAND rm -rf CMakeFiles
    COMMAND rm -rf cmake_install.cmake
    COMMAND rm -rf CMakeCache.txt
    COMMAND rm -rf Makefile
    COMMAND echo "All cleared."
)
