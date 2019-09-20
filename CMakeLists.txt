cmake_minimum_required(VERSION 3.12.0)
project(PowerMoniter VERSION 0.1.0)

set (CMAKE_CXX_STANDARD 11)

include(CTest)
enable_testing()

find_package(eCAL REQUIRED)


set(PowerMoniter_src
    main.cpp
    cserialport.cpp
    cmobject.cpp
)

add_executable(${PROJECT_NAME} ${PowerMoniter_src})

target_link_libraries(${PROJECT_NAME} eCAL::core)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
