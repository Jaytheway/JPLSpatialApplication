add_executable(JPLSpatialApplication)

jpl_enable_ipo()

#============================================================
# === Generate a .rc file from a .rc.in template, substituting
# variables like the version number and current year
string(TIMESTAMP CURRENT_YEAR "%Y")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SpatialApplication/resources.rc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/src/SpatialApplication/resources.rc"
  @ONLY
)

target_sources(JPLSpatialApplication PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/src/SpatialApplication/resources.rc"
)

#============================================================
# Collect sources/headers
file(GLOB_RECURSE JPLSE_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SpatialApplication/*.cpp"
  )
  
file(GLOB_RECURSE JPLSE_INTERNAL_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SpatialApplication/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/SpatialApplication/GUI/Embed/*.embed"
)

file(GLOB_RECURSE JPLSE_PUBLIC_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

# Attach to target
target_sources(JPLSpatialApplication
  PRIVATE
    ${JPLSE_SOURCES}

  PRIVATE FILE_SET internal_headers
      TYPE HEADERS
      BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/SpatialApplication"
      FILES ${JPLSE_INTERNAL_HEADERS}
)

# Only add the public file set if there are any files (avoids empty fileset warnings)
if (JPLSE_PUBLIC_HEADERS)
    target_sources(JPLSpatialApplication
        PUBLIC FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
            FILES ${JPLSE_PUBLIC_HEADERS}
    )
endif()

# === Put the files in a tree structure in IDE ===
get_target_property(_srcs JPLSpatialApplication SOURCES)
get_target_property(_ihdr JPLSpatialApplication HEADER_SET_internal_headers)

set(_all_files ${_srcs} ${_ihdr})

if (JPLSE_PUBLIC_HEADERS)
    get_target_property(_phdr JPLSpatialApplication HEADER_SET_public_headers)
    list(APPEND _all_files ${_phdr})
endif()

# Filter out the generated resource file from the 'TREE' group
set(_tree_files ${_all_files})
list(REMOVE_ITEM _tree_files "${CMAKE_CURRENT_BINARY_DIR}/src/SpatialApplication/resources.rc")

# Run the TREE grouping for everything in the source folder
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${_tree_files})

# Manually group the generated resource file
source_group("Generated" FILES "${CMAKE_CURRENT_BINARY_DIR}/src/SpatialApplication/resources.rc")


#============================================================
# C++ version
target_compile_features(JPLSpatialApplication PUBLIC cxx_std_20)

#============================================================
# 3rd-party dependencies
include(Dependencies.cmake)
jpl_setup_dependencie()

# Place the project in a folder in IDEs
set_target_properties(JPLSpatialApplication PROPERTIES FOLDER "Core")

# Global defines
target_compile_definitions(JPLSpatialApplication
  PUBLIC
    IMGUI_USER_CONFIG="${CMAKE_CURRENT_SOURCE_DIR}/src/SpatialApplication/ImGui/JPLImGuiConfig.h"
)


#============================================================
# Per-config defines (DEBUG/NDEBUG + JPL_TEST if tests are enabled)
target_compile_definitions(JPLSpatialApplication
  PRIVATE
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    $<$<BOOL:${BUILD_TESTING}>:JPL_TESTS>
  PUBLIC
    JPL_HAS_PATH_TRACING=1
    JPL_HAS_BEAM_TRACING=0
)

# Linux PIC
if(UNIX AND NOT APPLE)
  set_target_properties(JPLSpatialApplication PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

#============================================================
# Compiler flags & output directories

# x86/AVX2 vs x86/SSE2, skip on non-x86
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
  if(MSVC)
    target_compile_options(JPLSpatialApplication PRIVATE /arch:AVX2)
  else()
    target_compile_options(JPLSpatialApplication PRIVATE -mavx2 -mfma)
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i[3-6]86)$")
  if(MSVC)
    target_compile_options(JPLSpatialApplication PRIVATE /arch:SSE2)
  else()
    target_compile_options(JPLSpatialApplication PRIVATE -msse2)
  endif()
endif()

if (MSVC)
    target_compile_options(JPLSpatialApplication PRIVATE "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    target_compile_options(JPLSpatialApplication PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

# Set output directories
jpl_set_output_dirs(JPLSpatialApplication)

# Make JPLSpatialApplication the startup project in VS
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT JPLSpatialApplication)

#============================================================
# Resource handling

# Define the source assets directory relative to the current CMakeLists.txt
set(BUILD_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build_assets")
set(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")

# Include assets directory, so that .rc can find our icon
target_include_directories(JPLSpatialApplication PRIVATE "${BUILD_ASSETS_DIR}")

# Use a generator expression to find the target's output directory
# $<TARGET_FILE_DIR:JPLSpatialApplication> resolves to the directory where the executable/library is placed
# Append '/assets' to maintain the directory structure
set(DEST_DIR "$<TARGET_FILE_DIR:JPLSpatialApplication>/assets")

# Add a custom command to run after the target "JPLSpatialApplication" is built
add_custom_command(
    TARGET JPLSpatialApplication
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ASSETS_DIR}"
            "${DEST_DIR}"
    COMMENT "Copying assets directory for JPL Spatial Application to ${ASSETS_DIR}"
)

# Ensure the working directory for debugging in Visual Studio is set
# to the output directory where the executable is located, so that we can load assets using relative paths
set_target_properties(JPLSpatialApplication PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:JPLSpatialApplication>"
)

# Ensure it's a GUI app (remove the console window for Distrubution build)
set_target_properties(JPLSpatialApplication PROPERTIES WIN32_EXECUTABLE
    $<$<CONFIG:Distribution>:TRUE>
)
