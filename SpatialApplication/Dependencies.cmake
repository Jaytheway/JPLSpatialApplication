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
    endif()
endfunction()

function(fetch_MiniaudioCpp)
    if (NOT TARGET JPL::MiniaudioCpp)
        set(MINIAUDIOCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/MiniaudioCpp/MiniaudioCpp")
        add_subdirectory("${MINIAUDIOCPP_DIR}" "${CMAKE_BINARY_DIR}/MiniaudioCpp/MiniaudioCpp")  
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

function(jpl_setup_dependencie)

    # === ImGui ===
    #fetch_imgui()

    # === Walnut ===
    fetch_walnut() # Walnut provides ImGui for us

    # === JPLSpatial ===
    fetch_JPLSpatial()

    # === MiniaudioCpp ===
    fetch_MiniaudioCpp()

    # === implot ===
    fetch_implot()
    source_group("vendor\\implot" FILES ${implot_SOURCES})

    # === ImGuiFileDialog ===
    fetch_ImGuiFileDialog()
    source_group("vendor\\ImGuiFileDialog" FILES ${ImGuiFileDialog_SOURCES})

    # === Fonts ===
    fetch_fonts()
    source_group("vendor\\fonts" FILES ${fonts_SOURCES})

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
    )

    walnut_apply_defines(JPLSpatialApplication)
    if (WIN32)
        target_link_libraries(JPLSpatialApplication PRIVATE dwmapi)
    endif()

endfunction()
