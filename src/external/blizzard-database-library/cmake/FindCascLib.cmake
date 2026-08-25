# This file is part of Noggit3, licensed under GNU General Public License (version 3).

# adds target CascLib
FetchContent_Declare(
  casclib
  GIT_REPOSITORY https://gitlab.com/prophecy-rp/dependencies.git
  GIT_TAG        dep-casclib
)

FetchContent_GetProperties(casclib)
if(NOT casclib)
  MESSAGE(STATUS "Installing Casclib...")
  FetchContent_Populate(casclib)
  SET(CASCLIB_INCLUDE_DIR "${casclib_SOURCE_DIR}/includes")
  SET(CASCLIB_LIBRARY_DEBUG_DIR "${casclib_SOURCE_DIR}/lib/debug/x64")
  SET(CASCLIB_LIBRARY_RELEASE_DIR "${casclib_SOURCE_DIR}/lib/release/x64")
endif()

find_path (CASCLIB_INCLUDE_DIR CascLib.h CascPort.h)

find_library (_casc_debug_lib NAMES CascLibDAD CascLibDAS CascLibDUD CascLibDUS PATHS ${CASCLIB_LIBRARY_DEBUG_DIR})
find_library (_casc_release_lib NAMES CascLibRAD CascLibRAS CascLibRUD CascLibRUS PATHS ${CASCLIB_LIBRARY_RELEASE_DIR})
find_library (_casc_any_lib NAMES Casc Casclib CascLib)

set (CASC_LIBRARIES)
if (_casc_debug_lib AND _casc_release_lib)
  list (APPEND CASC_LIBRARIES debug ${_casc_debug_lib} optimized ${_casc_release_lib})
else()
  list (APPEND CASC_LIBRARIES ${_casc_any_lib})
endif()

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (CascLib DEFAULT_MSG CASC_LIBRARIES CASCLIB_INCLUDE_DIR)

mark_as_advanced (CASCLIB_INCLUDE_DIR _casc_debug_lib _casc_release_lib _casc_any_lib CASC_LIBRARIES)

add_library (CascLib INTERFACE)
target_link_libraries (CascLib INTERFACE ${CASC_LIBRARIES})
set_property  (TARGET CascLib APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${CASCLIB_INCLUDE_DIR})
set_property  (TARGET CascLib APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CASCLIB_INCLUDE_DIR})

#remove_definitions(-D_DLL)
#! \note on Windows, Casc tries to auto-link. There is no proper flag to disable that, so abuse this one.
target_compile_definitions (CascLib INTERFACE -DCASCLIB_NO_AUTO_LINK_LIBRARY)
