project(corerundll CXX)

################################################################################
# Source groups
################################################################################
set(Headers
    corerundll.h
    logger.h
)
source_group("Headers" FILES ${Headers})

set(Sources
    corerundll.cpp
    logger.cpp
)
source_group("Sources" FILES ${Sources})

set(ALL_FILES  ${Sources} ${Headers})

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM"
     OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64"
     OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32"
     OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64"))
    message(WARNING "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
endif()

################################################################################
# Target name
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    string(CONCAT TARGET_NAME
        $<$<CONFIG:Debug>:corerundll32>
        $<$<CONFIG:Release>:corerundll32>
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${PROJECT_NAME}>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    string(CONCAT TARGET_NAME
        $<$<CONFIG:Debug>:corerundll32>
        $<$<CONFIG:Release>:corerundll32>
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${PROJECT_NAME}>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    string(CONCAT TARGET_NAME
          $<$<CONFIG:Debug>:corerundll64>
        $<$<CONFIG:Release>:corerundll64>
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${PROJECT_NAME}>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    string(CONCAT TARGET_NAME
        $<$<CONFIG:Debug>:corerundll64>
        $<$<CONFIG:Release>:corerundll64>
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${PROJECT_NAME}>
    )
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${TARGET_NAME})

################################################################################
# Output directory
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    string(CONCAT OUT_DIR
        $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/bin.${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/>
        $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/bin.${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/>
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    string(CONCAT OUT_DIR
        $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/bin.${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/>
        $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/bin.${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/>
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    string(CONCAT OUT_DIR
        $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/bin.${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/>
        $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/bin.${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/>
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    string(CONCAT OUT_DIR
        $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/bin.X64/$<CONFIG>/>
        $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/bin.X64/$<CONFIG>/>
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>>
    )
endif()
set(ARCHIVE_OUT_DIR ${OUT_DIR})
set(ARCHIVE_OUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_NAME ${ARCHIVE_OUT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_OUT_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR})

################################################################################
# Include directories
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/.>
        $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/.>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/.>
        $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/.>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/.>
        $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/.>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/.>
        $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/.>
    )
endif()

################################################################################
# Compile definitions
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:DEBUG;_DEBUG;_EXPORTING;UNICODE;_UNICODE>
        $<$<CONFIG:Release>:_EXPORTING;UNICODE;_UNICODE>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:DEBUG;_DEBUG;_EXPORTING;UNICODE;_UNICODE>
        $<$<CONFIG:Release>:_EXPORTING;UNICODE;_UNICODE>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:DEBUG;_DEBUG;_EXPORTING;UNICODE;_UNICODE>
        $<$<CONFIG:Release>:NDEBUG;_EXPORTING;UNICODE;_UNICODE>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:DEBUG;_DEBUG;_EXPORTING;UNICODE;_UNICODE>
        $<$<CONFIG:Release>:NDEBUG;_EXPORTING;UNICODE;_UNICODE>
    )
endif()

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:/GL;/Od;/Oi;/GF;/MTd;/Gy;/W3;/WX;/Zi;/TP;/GR-;/wd4960;/wd4961;/wd4603;/wd4627;/wd4838;/wd4456;/wd4457;/wd4458;/wd4459;/wd4091;/d2Zi+;/Zm200;/ZH:SHA_256;/source-charset:utf-8;/homeparams;/EHa;/Zc:wchar_t->
            $<$<CONFIG:Release>:/GL;/O1;/Oi;/GF;/MT;/Gy;/W3;/WX;/TP;/GR-;/wd4960;/wd4961;/wd4603;/wd4627;/wd4838;/wd4456;/wd4457;/wd4458;/wd4459;/wd4091;/d2Zi+;/Zm200;/ZH:SHA_256;/source-charset:utf-8;/EHa;/Zc:wchar_t->
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:/GL;/Od;/Oi;/GF;/MTd;/Gy;/W3;/WX;/Zi;/TP;/GR-;/wd4960;/wd4961;/wd4603;/wd4627;/wd4838;/wd4456;/wd4457;/wd4458;/wd4459;/wd4091;/d2Zi+;/Zm200;/ZH:SHA_256;/source-charset:utf-8;/homeparams;/EHa;/Zc:wchar_t->
            $<$<CONFIG:Release>:/GL;/O1;/Oi;/GF;/MT;/Gy;/W3;/WX;/TP;/GR-;/wd4960;/wd4961;/wd4603;/wd4627;/wd4838;/wd4456;/wd4457;/wd4458;/wd4459;/wd4091;/d2Zi+;/Zm200;/ZH:SHA_256;/source-charset:utf-8;/EHa;/Zc:wchar_t->
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:/GL;/Od;/Oi;/GF;/MTd;/Gy;/W3;/WX;/Zi;/TP;/GR-;/wd4960;/wd4961;/wd4603;/wd4627;/wd4838;/wd4456;/wd4457;/wd4458;/wd4459;/wd4091;/d2Zi+;/Zm200;/ZH:SHA_256;/source-charset:utf-8;/homeparams;/EHa;/Zc:wchar_t->
            $<$<CONFIG:Release>:/GL;/O1;/Oi;/GF;/MT;/Gy;/W3;/WX;/TP;/GR-;/wd4960;/wd4961;/wd4603;/wd4627;/wd4838;/wd4456;/wd4457;/wd4458;/wd4459;/wd4091;/d2Zi+;/Zm200;/ZH:SHA_256;/source-charset:utf-8;/EHa;/Zc:wchar_t->
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:/GL;/Od;/Oi;/GF;/MTd;/Gy;/W3;/WX;/Zi;/TP;/GR-;/wd4960;/wd4961;/wd4603;/wd4627;/wd4838;/wd4456;/wd4457;/wd4458;/wd4459;/wd4091;/d2Zi+;/Zm200;/ZH:SHA_256;/source-charset:utf-8;/homeparams;/EHa;/Zc:wchar_t->
            $<$<CONFIG:Release>:/GL;/O1;/Oi;/GF;/MT;/Gy;/W3;/WX;/TP;/GR-;/wd4960;/wd4961;/wd4603;/wd4627;/wd4838;/wd4456;/wd4457;/wd4458;/wd4459;/wd4091;/d2Zi+;/Zm200;/ZH:SHA_256;/source-charset:utf-8;/EHa;/Zc:wchar_t->
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/LTCG /DEBUG /INCREMENTAL:NO /GUARD:CF /PDBCOMPRESS")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/LTCG /DEBUG /INCREMENTAL:NO /GUARD:CF /PDBCOMPRESS")
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/LTCG /DEBUG /INCREMENTAL:NO /GUARD:CF /PDBCOMPRESS")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/LTCG /DEBUG /INCREMENTAL:NO /GUARD:CF /PDBCOMPRESS")
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/LTCG /DEBUG /INCREMENTAL:NO /machine:x64 /GUARD:CF /PDBCOMPRESS")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/LTCG /DEBUG /INCREMENTAL:NO /machine:x64 /GUARD:CF /PDBCOMPRESS")
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/LTCG /DEBUG /INCREMENTAL:NO /machine:x86 /GUARD:CF /PDBCOMPRESS")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/LTCG /DEBUG /INCREMENTAL:NO /machine:x86 /GUARD:CF /PDBCOMPRESS")
    endif()
endif()

################################################################################
# Dependencies
################################################################################
# Link with other additional libraries.
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_link_libraries(${PROJECT_NAME}
          $<$<CONFIG:Debug>:advapi32$<SEMICOLON>oleaut32$<SEMICOLON>uuid$<SEMICOLON>user32$<SEMICOLON>libcmtd>
        $<$<CONFIG:Release>:advapi32$<SEMICOLON>oleaut32$<SEMICOLON>uuid$<SEMICOLON>user32$<SEMICOLON>libcmt>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    target_link_libraries(${PROJECT_NAME}
          $<$<CONFIG:Debug>:advapi32$<SEMICOLON>oleaut32$<SEMICOLON>uuid$<SEMICOLON>user32$<SEMICOLON>libcmtd>
        $<$<CONFIG:Release>:advapi32$<SEMICOLON>oleaut32$<SEMICOLON>uuid$<SEMICOLON>user32$<SEMICOLON>libcmt>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_link_libraries(${PROJECT_NAME}
          $<$<CONFIG:Debug>:advapi32$<SEMICOLON>oleaut32$<SEMICOLON>uuid$<SEMICOLON>user32$<SEMICOLON>libcmtd>
        $<$<CONFIG:Release>:advapi32$<SEMICOLON>oleaut32$<SEMICOLON>uuid$<SEMICOLON>user32$<SEMICOLON>libcmt>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_link_libraries(${PROJECT_NAME}
          $<$<CONFIG:Debug>:advapi32$<SEMICOLON>oleaut32$<SEMICOLON>uuid$<SEMICOLON>user32$<SEMICOLON>libcmtd>
        $<$<CONFIG:Release>:advapi32$<SEMICOLON>oleaut32$<SEMICOLON>uuid$<SEMICOLON>user32$<SEMICOLON>libcmt>
    )
endif()
