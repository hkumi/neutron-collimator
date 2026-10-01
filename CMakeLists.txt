cmake_minimum_required(VERSION 3.16...3.27)
project(Collimator)

find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(collimator collimator.cc ${sources} ${headers})
target_link_libraries(collimator ${Geant4_LIBRARIES})

# copy all macros (.mac) and ROOT scripts (.C) from the top folder into build
file(GLOB COPY_FILES ${PROJECT_SOURCE_DIR}/*.mac ${PROJECT_SOURCE_DIR}/*.C)
file(COPY ${COPY_FILES} DESTINATION ${PROJECT_BINARY_DIR})
