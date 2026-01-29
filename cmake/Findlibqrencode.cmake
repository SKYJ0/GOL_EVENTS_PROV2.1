# Findlibqrencode.cmake
find_path(LIBQRENCODE_INCLUDE_DIR NAMES qrencode.h PATH_SUFFIXES include)
find_library(LIBQRENCODE_LIBRARY NAMES qrencode libqrencode qrencode4 libqrencode4 PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libqrencode DEFAULT_MSG LIBQRENCODE_LIBRARY LIBQRENCODE_INCLUDE_DIR)

if(libqrencode_FOUND)
    set(libqrencode_LIBRARIES ${LIBQRENCODE_LIBRARY})
    set(libqrencode_INCLUDE_DIRS ${LIBQRENCODE_INCLUDE_DIR})
    
    if(NOT TARGET libqrencode::libqrencode)
        add_library(libqrencode::libqrencode UNKNOWN IMPORTED)
        set_target_properties(libqrencode::libqrencode PROPERTIES
            IMPORTED_LOCATION "${LIBQRENCODE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBQRENCODE_INCLUDE_DIR}"
        )
    endif()
endif()
mark_as_advanced(LIBQRENCODE_INCLUDE_DIR LIBQRENCODE_LIBRARY)
