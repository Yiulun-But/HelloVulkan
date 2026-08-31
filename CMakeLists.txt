# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.29.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")

project ("VulkanTutorial")

# Add option to enable/disable C++ 20 module
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support for Vulkan" OFF)

# Enable C++ module dependency scanning only if C++ 20 module is enabled
if(ENABLE_CPP20_MODULE)
  set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

# Include sub-projects.

find_package (glfw3 REQUIRED)
find_package (glm REQUIRED)
find_package (Vulkan 1.4.335 REQUIRED)      # Require Vulkan SDK version 1.4.335 or higher
find_package (tinyobjloader REQUIRED)

find_path(TINYGLTF_INCLUDE_DIR tiny_gltf_v3.h REQUIRED)

find_package (KTX REQUIRED)

# set up Vulkan C++ module only if enabled
if(ENABLE_CPP20_MODULE)
  add_library(VulkanCppModule)
  add_library(Vulkan::cppm ALIAS VulkanCppModule)

  target_compile_definitions(VulkanCppModule
          PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
  )
  target_include_directories(VulkanCppModule
          PUBLIC
          "${Vulkan_INCLUDE_DIR}"
  )
  target_link_libraries(VulkanCppModule
          PUBLIC
          Vulkan::Vulkan
  )

  set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

  # Add MSVC-specific compiler options for proper C++ module support
  if(MSVC)
    target_compile_options(VulkanCppModule PRIVATE
      /std:c++latest      # Use latest C++ standard for better module support
      /permissive-        # Standards conformance mode
      /Zc:__cplusplus     # Enable correct __cplusplus macro
      /EHsc               # Enable C++ exception handling
      /Zc:preprocessor    # Use conforming preprocessor
      /translateInclude   # Automatically translate #include to import for standard library
    )
  endif()

  target_sources(VulkanCppModule
          PUBLIC
          FILE_SET cxx_modules TYPE CXX_MODULES
          BASE_DIRS
          "${Vulkan_INCLUDE_DIR}"
          FILES
          "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
  )


  # Add the vulkan.cppm file directly as a source file
  target_sources(VulkanCppModule
          PRIVATE
          "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
  )
else()
  # Create a dummy interface library when C++ 20 module is disabled
  add_library(VulkanCppModule INTERFACE)
  add_library(Vulkan::cppm ALIAS VulkanCppModule)
  target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
  target_compile_definitions(VulkanCppModule
          INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
  )
endif()

find_package(stb REQUIRED)
set(STB_INCLUDEDIR ${stb_INCLUDE_DIRS})

add_executable (dxc::compiler IMPORTED)
find_program (DXC_COMPILER "dxc" HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
set_property (TARGET dxc::compiler PROPERTY IMPORTED_LOCATION "${DXC_COMPILER}")

function(add_shaders_target TARGET)
  cmake_parse_arguments(
    SHADER
    ""
    "OUTPUT_DIR"
    "SOURCES"
    ${ARGN}
  )

  if(NOT SHADER_OUTPUT_DIR)
    message(FATAL_ERROR "add_shaders_target: OUTPUT_DIR is required")
  endif()

  if(NOT SHADER_SOURCES)
    message(FATAL_ERROR "add_shaders_target: SOURCES is required")
  endif()

  set(shader_outputs)

  foreach(shader IN LISTS SHADER_SOURCES)
    get_filename_component(shader_name "${shader}" NAME)
    set(output_spv "${SHADER_OUTPUT_DIR}/${shader_name}.spv")

    if(shader_name MATCHES "\\.vert\\.hlsl$")
      set(shader_profile "vs_6_0")
    elseif(shader_name MATCHES "\\.frag\\.hlsl$")
      set(shader_profile "ps_6_0")
    elseif(shader_name MATCHES "\\.comp\\.hlsl$")
      set(shader_profile "cs_6_0")
    else()
      message(FATAL_ERROR "Unknown HLSL shader stage: ${shader_name}")
    endif()

    add_custom_command(
      OUTPUT "${output_spv}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUTPUT_DIR}"
      COMMAND dxc::compiler
      ARGS -spirv -T ${shader_profile} -E main -fspv-target-env=vulkan1.3 -Fo ${output_spv}  ${shader}
      DEPENDS "${shader}"
      COMMENT "Compiling shader ${shader_name}"
      VERBATIM
    )

    list(APPEND shader_outputs "${output_spv}")
  endforeach()

  add_custom_target(${TARGET} DEPENDS ${shader_outputs})
endfunction()

function(add_vkproject_shaders TARGET)
  cmake_parse_arguments(
    VKSHADER
    ""
    "OUTPUT_DIR"
    "SHADER_DIRS;SHADER_SOURCES"
    ${ARGN}
  )

  if(NOT TARGET ${TARGET})
    message(FATAL_ERROR "add_vkproject_shaders: target '${TARGET}' does not exist")
  endif()

  set(all_shader_sources ${VKSHADER_SHADER_SOURCES})

  foreach(shader_dir IN LISTS VKSHADER_SHADER_DIRS)
    file(GLOB dir_shaders CONFIGURE_DEPENDS
      "${shader_dir}/*.vert.hlsl"
      "${shader_dir}/*.frag.hlsl"
      "${shader_dir}/*.comp.hlsl"
      "${shader_dir}/*.geom.hlsl"
      "${shader_dir}/*.tesc.hlsl"
      "${shader_dir}/*.tese.hlsl"
    )
    list(APPEND all_shader_sources ${dir_shaders})
  endforeach()

  list(REMOVE_DUPLICATES all_shader_sources)

  if(NOT all_shader_sources)
    message(STATUS "add_vkproject_shaders: no shaders found for target '${TARGET}'")
    return()
  endif()

  if(VKSHADER_OUTPUT_DIR)
    set(shader_output_dir "${VKSHADER_OUTPUT_DIR}")
  else()
    set(shader_output_dir "${CMAKE_BINARY_DIR}/${TARGET}/shaders")
  endif()

  set(shader_target "${TARGET}_shaders")

  add_shaders_target(
    ${shader_target}
    OUTPUT_DIR "${shader_output_dir}"
    SOURCES ${all_shader_sources}
  )

  add_dependencies(${TARGET} ${shader_target})
endfunction()

function(add_vkproject_assets TARGET)
  cmake_parse_arguments(
    ASSET
    ""
    "ASSET_ROOT"
    "MODELS;TEXTURES"
    ${ARGN}
  )

  if(NOT TARGET ${TARGET})
    message(FATAL_ERROR "add_vkproject_assets: target '${TARGET}' does not exist")
  endif()

  if(NOT ASSET_ASSET_ROOT)
    set(ASSET_ASSET_ROOT "${CMAKE_SOURCE_DIR}/assets")
  endif()

  foreach(model IN LISTS ASSET_MODELS)
    add_custom_command(
      TARGET ${TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET}>/models"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${ASSET_ASSET_ROOT}/${model}"
              "$<TARGET_FILE_DIR:${TARGET}>/models/${model}"
      VERBATIM
    )
  endforeach()

  foreach(texture IN LISTS ASSET_TEXTURES)
    add_custom_command(
      TARGET ${TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET}>/textures"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${ASSET_ASSET_ROOT}/${texture}"
              "$<TARGET_FILE_DIR:${TARGET}>/textures/${texture}"
      VERBATIM
    )
  endforeach()
endfunction()

file(GLOB_RECURSE DEMO_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/demo/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/demo/*.hpp"
)

add_executable(demo ${DEMO_SOURCES})
set_target_properties(demo PROPERTIES CXX_STANDARD 20)
target_link_libraries(demo PRIVATE Vulkan::cppm glfw)
target_include_directories(demo PRIVATE "${CMAKE_SOURCE_DIR}/src")

# TinyGLTF
target_include_directories(demo PRIVATE ${TINYGLTF_INCLUDE_DIR})

add_vkproject_shaders(
  demo
  SHADER_DIRS
    "${CMAKE_SOURCE_DIR}/src/demo/shaders"
)

add_vkproject_assets(
  demo
  MODELS

  TEXTURES

)

# Add compile definition if C++ 20 module is enabled
if(ENABLE_CPP20_MODULE)
  target_compile_definitions(demo PRIVATE USE_CPP20_MODULES=1)
endif()

# Define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS to treat VK_ERROR_OUT_OF_DATE_KHR as a success code
target_compile_definitions(demo PRIVATE "VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS" )

if(WIN32)
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
        set_target_properties(demo PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${CHAPTER_NAME}")
  endif()
endif()