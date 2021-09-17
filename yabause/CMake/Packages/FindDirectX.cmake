# -----------------------------------------------------------------------------
# Find DirectX SDK
# Defines:
# DirectX_FOUND
# DirectX_INCLUDE_DIR
# DirectX_LIBRARIES
# DirectX_ROOT_DIR
# DirectX_LIBRARY_DIR
#
# DirectX_D3D11_FOUND
# DirectX_D3D11_INCLUDE_DIR
# DirectX_D3D11_LIBRARIES

if(WIN32) # The only platform it makes sense to check for DirectX SDK
    include(FindPkgMacros)
    findpkg_begin(DirectX)

    # Get path, convert backslashes as ${ENV_DXSDK_DIR}
    getenv_path(DXSDK_DIR)
    getenv_path(DIRECTX_HOME)
    getenv_path(DIRECTX_ROOT)
    getenv_path(DIRECTX_BASE)

    # construct search paths
    set(DirectX_PREFIX_PATH 
        "${DXSDK_DIR}" "${ENV_DXSDK_DIR}"
        "${DIRECTX_HOME}" "${ENV_DIRECTX_HOME}"
        "${DIRECTX_ROOT}" "${ENV_DIRECTX_ROOT}"
        "${DIRECTX_BASE}" "${ENV_DIRECTX_BASE}"
        "$ENV{ProgramFiles}/Windows Kits/10"
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/10"
        "$ENV{ProgramFiles}/Windows Kits/8.1"
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/8.1"
    )

    create_search_paths(DirectX)
    # redo search if prefix path changed
    clear_if_changed(DirectX_PREFIX_PATH
        DirectX_LIBRARY
        DirectX_INCLUDE_DIR
    )

    find_path(DirectX_INCLUDE_DIR NAMES d3d.h HINTS ${DirectX_INC_SEARCH_PATH} PATH_SUFFIXES "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/")
    # dlls are in DirectX_ROOT_DIR/Developer Runtime/x64|x86
    # lib files are in DirectX_ROOT_DIR/Lib/x64|x86
    if(CMAKE_CL_64)
        set(DirectX_LIBPATH_SUFFIX "x64")
    else()
        set(DirectX_LIBPATH_SUFFIX "x86")
    endif()

    set(_DX_LIB_PATH_SUFFIXES
        "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/${DirectX_LIBPATH_SUFFIX}" 
        ${DirectX_LIBPATH_SUFFIX}  
    )

    find_library(DirectX_LIBRARY NAMES d3d9 d3d10 d3d11 HINTS ${DirectX_LIB_SEARCH_PATH} PATH_SUFFIXES ${_DX_LIB_PATH_SUFFIXES})
    find_library(DirectX_D3DX9_LIBRARY NAMES d3dx9 HINTS ${DirectX_LIB_SEARCH_PATH} PATH_SUFFIXES ${_DX_LIB_PATH_SUFFIXES})
    find_library(DirectX_DXERR_LIBRARY NAMES DxErr HINTS ${DirectX_LIB_SEARCH_PATH} PATH_SUFFIXES ${_DX_LIB_PATH_SUFFIXES})
    find_library(DirectX_DXGUID_LIBRARY NAMES dxguid HINTS ${DirectX_LIB_SEARCH_PATH} PATH_SUFFIXES ${_DX_LIB_PATH_SUFFIXES})

    find_library(DirectX_INPUT8_LIBRARY dinput8 HINTS ${DirectX_LIB_SEARCH_PATH} PATH_SUFFIXES ${_DX_LIB_PATH_SUFFIXES})
    find_library(DirectX_XINPUT_LIBRARY xinput HINTS ${DirectX_LIB_SEARCH_PATH} PATH_SUFFIXES ${_DX_LIB_PATH_SUFFIXES})
	find_library(DirectX_SOUND_LIBRARY dsound HINTS ${DirectX_LIB_SEARCH_PATH} PATH_SUFFIXES ${_DX_LIB_PATH_SUFFIXES})

    # look for dxgi (needed by both 10 and 11)
    find_library(DirectX_DXGI_LIBRARY NAMES dxgi HINTS ${DirectX_LIB_SEARCH_PATH} PATH_SUFFIXES ${_DX_LIB_PATH_SUFFIXES})
    # look for d3dcompiler (needed by 11)
    find_library(DirectX_D3DCOMPILER_LIBRARY NAMES d3dcompiler HINTS ${DirectX_LIB_SEARCH_PATH} PATH_SUFFIXES ${_DX_LIB_PATH_SUFFIXES})

    findpkg_finish(DirectX)

    set(DirectX_LIBRARIES 
        ${DirectX_LIBRARY}
        ${DirectX_LIBRARIES} 
        ${DirectX_D3DX9_LIBRARY}
        ${DirectX_DXERR_LIBRARY}
        ${DirectX_DXGUID_LIBRARY}
    )

    mark_as_advanced(DirectX_D3DX9_LIBRARY DirectX_DXERR_LIBRARY DirectX_DXGUID_LIBRARY DirectX_DXGI_LIBRARY DirectX_D3DCOMPILER_LIBRARY)
  
    if (DirectX_FOUND)
        get_filename_component(DirectX_LIBRARY_DIR "${DirectX_LIBRARY}" PATH)

        # look for D3D11 components
        find_path(DirectX_D3D11_INCLUDE_DIR NAMES d3d11.h HINTS ${DirectX_INC_SEARCH_PATH} PATH_SUFFIXES "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/")
        find_library(DirectX_D3D11_LIBRARY NAMES d3d11 HINTS ${DirectX_LIB_SEARCH_PATH} PATH_SUFFIXES ${_DX_LIB_PATH_SUFFIXES})
        find_library(DirectX_D3DX11_LIBRARY NAMES d3dx11 HINTS ${DirectX_LIB_SEARCH_PATH} PATH_SUFFIXES ${_DX_LIB_PATH_SUFFIXES})	
        if (DirectX_D3D11_INCLUDE_DIR AND DirectX_D3D11_LIBRARY)
            set(DirectX_D3D11_FOUND TRUE)
            set(DirectX_D3D11_INCLUDE_DIR ${DirectX_D3D11_INCLUDE_DIR})
            set(DirectX_D3D11_LIBRARIES ${DirectX_D3D11_LIBRARIES}
                ${DirectX_D3D11_LIBRARY}
                ${DirectX_D3DX11_LIBRARY}
                ${DirectX_DXGI_LIBRARY}
                ${DirectX_DXERR_LIBRARY}
                ${DirectX_DXGUID_LIBRARY}
                ${DirectX_D3DCOMPILER_LIBRARY}
            )	
        endif()
        mark_as_advanced(DirectX_D3D11_INCLUDE_DIR DirectX_D3D11_LIBRARY DirectX_D3DX11_LIBRARY)
    endif()
endif()
