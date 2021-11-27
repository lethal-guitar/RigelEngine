add_library(RetroWaveLib STATIC
    Board/OPL3.c
    Protocol/Serial.c
    RetroWave.c
)

if(WIN32)
    target_sources(RetroWaveLib
        PRIVATE
        Platform/Win32_SerialPort.c
    )
else()
    target_sources(RetroWaveLib
        PRIVATE
        Platform/POSIX_SerialPort.c
    )
endif()

target_include_directories(RetroWaveLib
    INTERFACE
    ${CMAKE_SOURCE_DIR}/3rd_party/retrowave-opl3
)

