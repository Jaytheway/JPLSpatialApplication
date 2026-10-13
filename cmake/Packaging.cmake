# Stage only the application, not the install targets of its dependencies.
install(TARGETS JPLSpatialApplication
  BUNDLE DESTINATION . COMPONENT Runtime
  RUNTIME DESTINATION . COMPONENT Runtime)

set(resource_destination .)
if(APPLE)
  set(resource_destination "JPLSpatialApplication.app/Contents/Resources")
else()
  install(DIRECTORY "${PROJECT_SOURCE_DIR}/SpatialApplication/assets" DESTINATION . COMPONENT Runtime)
endif()
install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" DESTINATION "${resource_destination}" COMPONENT Runtime)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/licenses" DESTINATION "${resource_destination}" COMPONENT Runtime)
if(APPLE)
  # Sign only after all bundle resources have been installed.
  install(SCRIPT "${CMAKE_CURRENT_LIST_DIR}/InstallMacBundle.cmake" COMPONENT Runtime)
endif()

set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-x64")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_INSTALL_CMAKE_PROJECTS "${PROJECT_BINARY_DIR};${PROJECT_NAME};Runtime;/")
if(UNIX AND NOT APPLE)
  set(CPACK_GENERATOR TGZ)
else()
  set(CPACK_GENERATOR ZIP)
endif()
include(CPack)
