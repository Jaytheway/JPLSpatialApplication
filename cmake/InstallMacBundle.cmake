# Executed by cmake --install / CPack, after the .app has been staged.
set(app "${CMAKE_INSTALL_PREFIX}/JPLSpatialApplication.app")
set(vk_sdk "$ENV{VULKAN_SDK}")
set(llvm "$ENV{LLVM_ROOT}")
set(resources "${app}/Contents/Resources")
file(MAKE_DIRECTORY "${app}/Contents/Frameworks" "${resources}/vulkan/icd.d")

# MoltenVK is loaded through this manifest, so dependency scanning cannot find it.
file(COPY_FILE "${vk_sdk}/lib/libMoltenVK.dylib" "${app}/Contents/Frameworks/libMoltenVK.dylib")
file(READ "${vk_sdk}/share/vulkan/icd.d/MoltenVK_icd.json" manifest)
string(JSON manifest SET "${manifest}" ICD library_path "\"../../../Frameworks/libMoltenVK.dylib\"")
file(WRITE "${resources}/vulkan/icd.d/MoltenVK_icd.json" "${manifest}\n")

# CMake copies linked non-system dylibs (including libc++ and Vulkan), rewrites
# their paths relative to the app, and verifies the resulting dependencies.
include(BundleUtilities)
fixup_bundle("${app}" "${app}/Contents/Frameworks/libMoltenVK.dylib"
             "${vk_sdk}/lib;${llvm}/lib/c++;${llvm}/lib/unwind")
# GLFW's fallback loader lookup searches Contents/MacOS.
if(NOT EXISTS "${app}/Contents/Frameworks/libvulkan.1.dylib")
  message(FATAL_ERROR "Vulkan loader was not bundled")
endif()
file(CREATE_LINK "../Frameworks/libvulkan.1.dylib"
     "${app}/Contents/MacOS/libvulkan.1.dylib" SYMBOLIC)

# Retain redistribution notices supplied with the bundled runtimes.
foreach(root IN ITEMS "${vk_sdk}" "${llvm}")
  get_filename_component(label "${root}" NAME)
  file(GLOB_RECURSE notices RELATIVE "${root}"
       "${root}/*LICENSE*" "${root}/*license*" "${root}/*NOTICE*" "${root}/*copyright*")
  foreach(notice IN LISTS notices)
    get_filename_component(directory "${notice}" DIRECTORY)
    file(INSTALL "${root}/${notice}" DESTINATION "${resources}/licenses/${label}/${directory}")
  endforeach()
endforeach()

# Rewriting Mach-O files invalidates their old signatures. Ad-hoc signing needs
# no credentials; public Developer ID signing/notarization is separate.
execute_process(COMMAND /usr/bin/codesign --force --deep --sign - "${app}"
                COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND /usr/bin/codesign --verify --deep --strict "${app}"
                COMMAND_ERROR_IS_FATAL ANY)
