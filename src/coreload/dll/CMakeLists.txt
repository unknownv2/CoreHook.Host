include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    common
    logging
    json/casablance/include
    )

set(CORELOAD_DLL_SOURCES 
    coreload.cc
    coreload.rc    
    )

add_library(coreload_dll SHARED ${CORELOAD_DLL_SOURCES})
target_compile_definitions(coreload_dll PRIVATE _USRDLL=1)

target_link_libraries(coreload_dll coreload)
set_target_properties(coreload_dll PROPERTIES OUTPUT_NAME ${CORELOAD_DLL_NAME})