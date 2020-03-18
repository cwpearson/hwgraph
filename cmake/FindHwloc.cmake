set(HWLOC_DIR "" CACHE PATH "Root directory containing hwloc")

find_package(PkgConfig)

if(HWLOC_DIR)
  set(ENV{PKG_CONFIG_PATH} "${HWLOC_DIR}/lib/pkgconfig" $ENV{PKG_CONFIG_PATH})
endif()
pkg_check_modules(PC_Hwloc QUIET hwloc)


find_path(HWLOC_INCLUDE_DIR
  NAMES hwloc.h
  PATHS 
  ${PC_Hwloc_INCLUDE_DIRS}
  /usr/include
  PATH_SUFFIXES Hwloc
)
find_library(HWLOC_LIBRARY
  NAMES hwloc
  PATHS 
  ${PC_Hwloc_LIBRARY_DIRS}
  /usr/lib/x86_64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Hwloc

    "Could NOT find Hwloc; Options depending on Hwloc will be disabled
  if needed, please specify the library location
    - using HWLOC_DIR [${HWLOC_DIR}]
    - or a combination of HWLOC_INCLUDE_DIR [${HWLOC_INCLUDE_DIR}] and HWLOC_LIBRARY [${HWLOC_LIBRARY}]"
    HWLOC_LIBRARY HWLOC_INCLUDE_DIR )

if(Hwloc_FOUND)
  set(Hwloc_LIBRARIES ${HWLOC_LIBRARY})
  set(Hwloc_INCLUDE_DIRS ${HWLOC_INCLUDE_DIR})
  set(Hwloc_DEFINITIONS ${PC_Hwloc_CFLAGS_OTHER})
endif()

if(Hwloc_FOUND AND NOT TARGET Hwloc::Hwloc)
  add_library(Hwloc::Hwloc UNKNOWN IMPORTED)
  set_target_properties(Hwloc::Hwloc PROPERTIES
    IMPORTED_LOCATION "${HWLOC_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_Foo_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${HWLOC_INCLUDE_DIR}"
  )
endif()


mark_as_advanced(FORCE HWLOC_DIR HWLOC_INCLUDE_DIR HWLOC_LIBRARY)