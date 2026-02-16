# Add include directory
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/drvr/inc)

# Add source files
target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mainBma423.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drvr/src/zbusBma423.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drvr/src/msgqBma423.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drvr/src/wdtBma423.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drvr/src/zephyrBma423.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drvr/src/bma4.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drvr/src/bma423.c
)

# Conditionally add shell source
if(CONFIG_BMA423_SHELL)
    target_sources(app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/shell/src/shellBma423.c
    )
endif()