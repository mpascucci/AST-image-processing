file(GLOB unit_test_source_files unit_tests/*.cpp)
add_executable(fullExample example/fullExample.cpp test_utils.cpp)
add_executable(runUnitTests ${unit_test_source_files})

include_directories(include)

#/* ----------------------------- INCLUDE FOLDERS ---------------------------- */
# add folder to the search path for include files when runinng test
include_directories("${PROJECT_SOURCE_DIR}/astimplib/src")
include_directories("${PROJECT_BINARY_DIR}/astimplib/include")

# =============================================================================
# copy test images
# see: https://stackoverflow.com/questions/15319859/how-to-concatenate-two-strings-in-c
# =============================================================================
file(GLOB_RECURSE test_data "images/*")

message("test_images:")
foreach(df ${test_data})
    message("- " ${df})
    # copy images to build dir
    if(${df} MATCHES "((jp|pn)g|bmp)$")
        file(COPY ${df} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
        set(df_file_path ${CMAKE_CURRENT_BINARY_DIR}/${df})
    else()
        set(df_file_path ${CMAKE_CURRENT_SOURCE_DIR}/${df})
    endif()
endforeach()
message(" ")

set(TEST_IMG_PATH \"${CMAKE_CURRENT_BINARY_DIR}/\")
configure_file("include/test_config.h.in" "include/test_config.h" @ONLY)

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/include ) # make test_config.h visible for compiler


#/* ----------------------------- LINK LIBRARIES ----------------------------- */

# GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

target_link_libraries(fullExample astimp)

target_link_libraries(runUnitTests astimp ${OpenCV_LIBS} ${GTEST_BOTH_LIBRARIES} pthread)

#/* --------------------------------- INSTALL -------------------------------- */
install(TARGETS fullExample DESTINATION "${CUSTOM_INSTALL_DIR}")

#/* ---------------------------------- TESTS --------------------------------- */
include(GoogleTest)
gtest_discover_tests(runUnitTests)
