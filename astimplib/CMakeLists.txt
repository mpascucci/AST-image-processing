#/* --------------------------------- VERSION -------------------------------- */
set (ASTAPP_IMPROC_VERSION_MAJOR 1)
set (ASTAPP_IMPROC_VERSION_MINOR 4)
configure_file("include/astimp_version.hpp.in" "include/astimp_version.hpp")

include_directories(include)

set(PELLET_LABEL_TFLITE_MODEL
    ${CMAKE_CURRENT_SOURCE_DIR}/../pellet_labels/models/ensemble_model.tflite)
set(GENERATE_CPP_MODEL_PY ${CMAKE_CURRENT_SOURCE_DIR}/../pellet_labels/generate_cpp_model.py)

add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/gen/pellet_label_tflite_model.cpp
    DEPENDS ${GENERATE_CPP_MODEL_PY}
    DEPENDS ${PELLET_LABEL_TFLITE_MODEL}
    COMMAND ${CMAKE_COMMAND} -E make_directory gen
    COMMAND python3 ${GENERATE_CPP_MODEL_PY} ${PELLET_LABEL_TFLITE_MODEL} > gen/pellet_label_tflite_model.cpp)

add_library(
    astimp SHARED
    gen/pellet_label_tflite_model.cpp
    include/astimp.hpp
    include/improc-api.hpp
    include/pellet_label_recognition.hpp
    include/pellet_label_tflite_model.hpp
        include/utils.hpp
    src/astimp.cpp
    src/improc-api.cpp
    src/pellet_label_recognition.cpp
    src/pellet_label_recognition_ml.cpp
        src/utils.cpp)

if(NOT ANDROID_ABI)
    if(APPLE)
        set(TFLITE_LIB osx_x86_64/libtensorflowlite_c.dylib)
    else()
        set(TFLITE_LIB linux_x86_64/libtensorflowlite_c.so)
    endif()
else()
    set(TFLITE_LIB ${ANDROID_ABI}/libtensorflowlite_jni.so)
endif()

target_link_libraries( # Specifies the target library.
    astimp
    #${log-lib}     # Links the target library to the log library included in the NDK.
    ${OpenCV_LIBS}
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tensorflow/lib/${TFLITE_LIB}
)


#/* --------------------------------- INSTALL -------------------------------- */
# message("Installing astimp lib and headers in ${CMAKE_INSTALL_PREFIX}")
# install(TARGETS astimp)
# install(FILES include/astimp.hpp DESTINATION "include")
# install(FILES include/utils.hpp DESTINATION "include")
# install(FILES include/pellet_label_recognition.hpp DESTINATION "include")
# install(FILES include/astExceptions.hpp DESTINATION "include")
# install(FILES include/stand_labels.hpp DESTINATION "include")
# install(FILES ${CMAKE_BINARY_DIR}/astimplib/include/astimp_version.hpp DESTINATION "include")

message("Install dir: ${CUSTOM_INSTALL_DIR}")
install(TARGETS astimp DESTINATION "${CUSTOM_INSTALL_DIR}/lib")
install(FILES include/astimp.hpp DESTINATION "${CUSTOM_INSTALL_DIR}/include")
install(FILES include/utils.hpp DESTINATION "${CUSTOM_INSTALL_DIR}/include")
install(FILES include/pellet_label_recognition.hpp DESTINATION "${CUSTOM_INSTALL_DIR}/include")
install(FILES include/astExceptions.hpp DESTINATION "${CUSTOM_INSTALL_DIR}/include")
install(FILES include/stand_labels.hpp DESTINATION "${CUSTOM_INSTALL_DIR}/include")
install(FILES ${CMAKE_BINARY_DIR}/astimplib/include/astimp_version.hpp DESTINATION "${CUSTOM_INSTALL_DIR}/include")
## NOTE uninstall with:
## build$ xargs rm < install_manifest.txt