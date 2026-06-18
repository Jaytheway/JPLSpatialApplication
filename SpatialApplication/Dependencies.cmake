include(ExternalProject)

function(fetch_imgui)
    if (NOT TARGET ImGui)
        CPMAddPackage(
            NAME ImGui
            GITHUB_REPOSITORY StudioCherno/imgui
            GIT_TAG hazel
            GIT_SUBMODULES_RECURSE YES
            GIT_SHALLOW TRUE
            DOWNLOAD_ONLY YES
        )
        set(ImGui_SOURCE_DIR "${ImGui_SOURCE_DIR}" PARENT_SCOPE)
    endif()
endfunction()

function(fetch_walnut)
    if (NOT TARGET Walnut::Walnut)
        add_subdirectory("vendor/Walnut")
    endif()
endfunction()

function(fetch_JPLSpatial)
    if (NOT TARGET JPLSpatial)
        set(JPL_SPATIAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/JPLSpatial")
        add_subdirectory("${JPL_SPATIAL_DIR}" "${CMAKE_BINARY_DIR}/JPLSpatial")  

        get_target_property(LIB_SOURCES JPLSpatial SOURCES)
        
        # Remove ErrorReporting.cpp
        list(REMOVE_ITEM LIB_SOURCES "${JPL_SPATIAL_DIR}/Spatialization/src/Spatialization/ErrorReporting.cpp")
        set_target_properties(JPLSpatial PROPERTIES SOURCES "${LIB_SOURCES}")

        # Pass JPL Spatial's core include path to the parent scope so other dependencies can use it
        set(JPL_CORE_INCLUDE_PATH "${JPL_SPATIAL_DIR}/Spatialization/include" PARENT_SCOPE)
    endif()
endfunction()

function(fetch_MiniaudioCpp JPL_CORE_INCLUDE_PATH)
    if (NOT TARGET JPL::MiniaudioCpp)
        set(MINIAUDIOCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/MiniaudioCpp/MiniaudioCpp")
        add_subdirectory("${MINIAUDIOCPP_DIR}" "${CMAKE_BINARY_DIR}/MiniaudioCpp/MiniaudioCpp")
        
        # Let MiniadioCpp use JPL Spatial's core includes
        target_compile_definitions(MiniaudioCpp PRIVATE
           "JPL_CORE_INCLUDE=\"${JPL_CORE_INCLUDE_PATH}/JPLSpatial/Core.h\""
           "JPL_ERROR_REPORTING_INCLUDE=\"${JPL_CORE_INCLUDE_PATH}/JPLSpatial/ErrorReporting.h\""
        )
    endif()
endfunction()

function(fetch_FFTConvolver)
    if (NOT TARGET FFTConvolver)
        CPMAddPackage(
            NAME FFTConvolver
            GITHUB_REPOSITORY falkTX/FFTConvolver
            GIT_TAG non-uniform
            GIT_SUBMODULES_RECURSE YES
            GIT_SHALLOW TRUE
            DOWNLOAD_ONLY YES
        )
        add_library(FFTConvolver INTERFACE)
        set(FFTCONVOLVER_SOURCES
            "${FFTConvolver_SOURCE_DIR}/AudioFFT.h"
            "${FFTConvolver_SOURCE_DIR}/AudioFFT.cpp"
        )
        target_sources(FFTConvolver INTERFACE
           ${FFTCONVOLVER_SOURCES}
        )
        target_include_directories(FFTConvolver INTERFACE
            "${FFTConvolver_SOURCE_DIR}"
        )
        set(FFTConvolver_SOURCES "${FFTCONVOLVER_SOURCES}" PARENT_SCOPE)
    endif()
endfunction()

function(fetch_implot)
    if (NOT TARGET implot)
        CPMAddPackage(
            NAME implot
            GITHUB_REPOSITORY epezent/implot
            GIT_TAG master
            GIT_SUBMODULES_RECURSE YES
            GIT_SHALLOW TRUE
            DOWNLOAD_ONLY YES
        )
        add_library(implot INTERFACE)
        set(IMPLOT_SOURCES
            "${implot_SOURCE_DIR}/implot.h"
            "${implot_SOURCE_DIR}/implot.cpp"
            "${implot_SOURCE_DIR}/implot_demo.cpp"
            "${implot_SOURCE_DIR}/implot_internal.h"
            "${implot_SOURCE_DIR}/implot_items.cpp"
        )
        target_sources(implot INTERFACE
           ${IMPLOT_SOURCES}
        )
        target_include_directories(implot INTERFACE
            "${implot_SOURCE_DIR}"
        )
        set(implot_SOURCES "${IMPLOT_SOURCES}" PARENT_SCOPE)
    endif()
endfunction()

function(fetch_ImGuiFileDialog)
    if (NOT TARGET ImGuiFileDialog)
         CPMAddPackage(
            NAME ImGuiFileDialog
            GITHUB_REPOSITORY aiekick/ImGuiFileDialog
            GIT_TAG master
            GIT_SUBMODULES_RECURSE YES
            GIT_SHALLOW TRUE
            DOWNLOAD_ONLY YES
        )
        add_library(ImGuiFileDialog INTERFACE)
        set(IMGUIFILEDIALOG_SOURCES
            "${ImGuiFileDialog_SOURCE_DIR}/ImGuiFileDialog.h"
            "${ImGuiFileDialog_SOURCE_DIR}/ImGuiFileDialog.cpp"
            "${ImGuiFileDialog_SOURCE_DIR}/ImGuiFileDialogConfig.h"
        )
        target_sources(ImGuiFileDialog INTERFACE
           ${IMGUIFILEDIALOG_SOURCES}
        )
        target_include_directories(ImGuiFileDialog INTERFACE
            "${ImGuiFileDialog_SOURCE_DIR}"
        )
        target_compile_definitions(ImGuiFileDialog INTERFACE
            CUSTOM_IMGUIFILEDIALOG_CONFIG="${CMAKE_CURRENT_SOURCE_DIR}/src/SpatialApplication/ImGui/JPLImGuiFileDialogConfig.h"
        )
        set(ImGuiFileDialog_SOURCES "${IMGUIFILEDIALOG_SOURCES}" PARENT_SCOPE)
    endif()
endfunction()

function(fetch_fonts)
    if (NOT TARGET Fonts)
        add_library(Fonts INTERFACE)

        file(GLOB_RECURSE FONTS_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/fonts/iconsfont.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/fonts/iconsfont.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/fonts/*.embed"
        )
        target_sources(Fonts INTERFACE ${FONTS_SOURCES})
        target_include_directories(Fonts INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/vendor")
        set(fonts_SOURCES "${FONTS_SOURCES}" PARENT_SCOPE)
    endif()
endfunction()

function(fetch_concurrentqueue)
    if (NOT TARGET concurrentqueue)
        FetchContent_Declare(
            concurrentqueue
            URL "https://raw.githubusercontent.com/cameron314/concurrentqueue/refs/heads/master/concurrentqueue.h"
            DOWNLOAD_NO_EXTRACT TRUE
        )
        FetchContent_MakeAvailable(concurrentqueue)

        add_library(concurrentqueue INTERFACE)
        target_sources(concurrentqueue INTERFACE "${concurrentqueue_SOURCE_DIR}/concurrentqueue.h")
        target_include_directories(concurrentqueue INTERFACE ${concurrentqueue_SOURCE_DIR})
        set(concurrentqueue_SOURCES "${concurrentqueue_SOURCE_DIR}/concurrentqueue.h" PARENT_SCOPE)
    endif()
endfunction()

function(fetch_magicenum)
    if (NOT TARGET magic_enum)
        CPMAddPackage(
            NAME magic_enum
            GITHUB_REPOSITORY Neargye/magic_enum
            GIT_TAG master
            GIT_SHALLOW TRUE
            DOWNLOAD_ONLY YES
        )
        add_library(magic_enum INTERFACE)
        set(MAGICENUM_SOURCES
            "${magic_enum_SOURCE_DIR}/include/magic_enum/magic_enum.hpp"
            "${magic_enum_SOURCE_DIR}/include/magic_enum/magic_enum_all.hpp"
            "${magic_enum_SOURCE_DIR}/include/magic_enum/magic_enum_containers.hpp"
            "${magic_enum_SOURCE_DIR}/include/magic_enum/magic_enum_flags.hpp"
            "${magic_enum_SOURCE_DIR}/include/magic_enum/magic_enum_format.hpp"
            "${magic_enum_SOURCE_DIR}/include/magic_enum/magic_enum_fuse.hpp"
            "${magic_enum_SOURCE_DIR}/include/magic_enum/magic_enum_iostream.hpp"
            "${magic_enum_SOURCE_DIR}/include/magic_enum/magic_enum_switch.hpp"
            "${magic_enum_SOURCE_DIR}/include/magic_enum/magic_enum_utility.hpp"
        )
        target_sources(magic_enum INTERFACE "${MAGICENUM_SOURCES}")
        target_include_directories(magic_enum INTERFACE ${magic_enum_SOURCE_DIR}/include)
        set(magicenum_SOURCES "${MAGICENUM_SOURCES}" PARENT_SCOPE)
    endif()
endfunction()

function(jpl_setup_dependencie)

    # === ImGui ===
    #fetch_imgui()

    # === Walnut ===
    fetch_walnut() # Walnut provides ImGui for us

    # === JPLSpatial ===
    fetch_JPLSpatial()

    # === MiniaudioCpp ===
    fetch_MiniaudioCpp(${JPL_CORE_INCLUDE_PATH})
    # Pass the core include path to MiniaudioCpp
    # so it can use JPL Spatial's error reporting implementation instead of its own
    
    # === FFTConvolver ===
    fetch_FFTConvolver()
    source_group("vendor\\FFTConvolver" FILES ${FFTConvolver_SOURCES})

    # === implot ===
    fetch_implot()
    source_group("vendor\\implot" FILES ${implot_SOURCES})

    # === ImGuiFileDialog ===
    fetch_ImGuiFileDialog()
    source_group("vendor\\ImGuiFileDialog" FILES ${ImGuiFileDialog_SOURCES})

    # === Fonts ===
    fetch_fonts()
    source_group("vendor\\fonts" FILES ${fonts_SOURCES})

    # === concurrentqueue ===
    fetch_concurrentqueue()
    source_group("vendor\\concurrentqueue" FILES ${concurrentqueue_SOURCES})

    # === magic_enum ===
    fetch_magicenum()
    source_group("vendor\\magic_enum" FILES ${magicenum_SOURCES})

    # Put 3rd-party targets under the "Dependencies" folder in VS
    function(_put_in_folder tgt folder)
        get_target_property(_aliased "${tgt}" ALIASED_TARGET)
        if(_aliased)
            set_target_properties("${_aliased}" PROPERTIES FOLDER "${folder}")
        else()
            set_target_properties("${tgt}" PROPERTIES FOLDER "${folder}")
        endif()
    endfunction()
  
    set(_walnut_deps glm_header_only glfw ImGui implot)
    foreach(_t IN LISTS _walnut_deps)
    if(TARGET "${_t}")
        _put_in_folder("${_t}" "Dependencies/WalnutDeps")
        endif()
    endforeach()

    set(_dep_targets Walnut JPLSpatial MiniaudioCpp)
    foreach(_t IN LISTS _dep_targets)
        if(TARGET "${_t}")
            _put_in_folder("${_t}" "Dependencies")
        endif()
    endforeach()
      
    # Link the libaries
    target_link_libraries(JPLSpatialApplication PRIVATE
        ImGui
        Walnut::Walnut
        JPLSpatial
        MiniaudioCpp
        implot
        ImGuiFileDialog
        Fonts
        FFTConvolver
        concurrentqueue
        magic_enum
    )

    walnut_apply_defines(JPLSpatialApplication)
    if (WIN32)
        target_link_libraries(JPLSpatialApplication PRIVATE dwmapi)
    endif()

endfunction()
