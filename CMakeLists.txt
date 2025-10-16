cmake_minimum_required(VERSION 3.10)
project(TobiiEyeTracking)

# C++17標準を使用
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure MSVC treats source files as UTF-8 to allow Unicode string literals in source code
if (MSVC)
    add_compile_options(/utf-8)
endif()

# Windowsプラットフォームのチェック
if(NOT WIN32)
    message(FATAL_ERROR "このプロジェクトはWindows専用です")
endif()

# Tobii SDKのパスを設定（環境に合わせて変更してください）
set(TOBII_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tobii_sdk" CACHE PATH "Tobii SDKのパス")

# Stream Engine用の設定
set(TOBII_STREAM_ENGINE_INCLUDE "${TOBII_SDK_PATH}/stream_engine/include")
set(TOBII_STREAM_ENGINE_LIB "${TOBII_SDK_PATH}/stream_engine/lib/tobii")

# Game Integration API用の設定
set(TOBII_GAME_INTEGRATION_INCLUDE "${TOBII_SDK_PATH}/game_integration/include")
set(TOBII_GAME_INTEGRATION_LIB "${TOBII_SDK_PATH}/game_integration/lib")

# Pro SDK用の設定（研究用）
set(TOBII_PRO_SDK_INCLUDE "${TOBII_SDK_PATH}/pro_sdk/include")
set(TOBII_PRO_SDK_LIB "${TOBII_SDK_PATH}/pro_sdk/lib")

# アーキテクチャの判定
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH "x64")
else()
    set(ARCH "x86")
endif()

# Stream Engineを使用した実行ファイル
# add_executable(tobii_stream_engine src/main.cpp)

# Game Integration APIを使用した実行ファイル
# add_executable(tobii_game_integration src/main_game_integration.cpp)

# 視線データレコーダー
# add_executable(gaze_recorder src/gaze_recorder.cpp)

# Tobii Pro SDK リアルタイムビューア（研究用）
add_executable(tobii_pro_research src/tobii_pro_research.cpp)

target_include_directories(tobii_pro_research PRIVATE ${TOBII_PRO_SDK_INCLUDE})

target_link_libraries(tobii_pro_research 
    "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.lib"
)

# Tobii Pro SDK 高性能レコーダー（研究用）
add_executable(tobii_pro_recorder src/tobii_pro_recorder.cpp)

target_include_directories(tobii_pro_recorder PRIVATE ${TOBII_PRO_SDK_INCLUDE})

target_link_libraries(tobii_pro_recorder 
    "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.lib"
)

# DLLのコピー設定（ビルド後に実行ファイルと同じディレクトリにコピー）
# add_custom_command(TARGET tobii_stream_engine POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     "${TOBII_STREAM_ENGINE_LIB}/${ARCH}/tobii_stream_engine.dll"
#     $<TARGET_FILE_DIR:tobii_stream_engine>
# )

# add_custom_command(TARGET tobii_game_integration POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     "${TOBII_GAME_INTEGRATION_LIB}/${ARCH}/tobii_gameintegration.dll"
#     $<TARGET_FILE_DIR:tobii_game_integration>
# )

# add_custom_command(TARGET gaze_recorder POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     "${TOBII_GAME_INTEGRATION_LIB}/${ARCH}/tobii_gameintegration.dll"
#     $<TARGET_FILE_DIR:gaze_recorder>
# )

add_custom_command(TARGET tobii_pro_research POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.dll"
    $<TARGET_FILE_DIR:tobii_pro_research>
)

add_custom_command(TARGET tobii_pro_recorder POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.dll"
    $<TARGET_FILE_DIR:tobii_pro_recorder>
)

# インストール設定
install(TARGETS tobii_pro_research tobii_pro_recorder
    RUNTIME DESTINATION bin
)

install(FILES "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.dll" DESTINATION bin)

# Tobii Pro SDK Overlay viewer
add_executable(tobii_pro_overlay WIN32 src/tobii_pro_overlay.cpp)

target_include_directories(tobii_pro_overlay PRIVATE ${TOBII_PRO_SDK_INCLUDE})

target_link_libraries(tobii_pro_overlay "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.lib" gdi32 user32)

add_custom_command(TARGET tobii_pro_overlay POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.dll" $<TARGET_FILE_DIR:tobii_pro_overlay>)

install(TARGETS tobii_pro_overlay RUNTIME DESTINATION bin)

# reading sequence
add_executable(tobii_pro_sequence WIN32 src/tobii_pro_sequence.cpp)

target_include_directories(tobii_pro_sequence PRIVATE ${TOBII_PRO_SDK_INCLUDE})

target_link_libraries(tobii_pro_sequence "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.lib" gdi32 user32)

add_custom_command(TARGET tobii_pro_sequence POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.dll" $<TARGET_FILE_DIR:tobii_pro_sequence>)

install(TARGETS tobii_pro_sequence RUNTIME DESTINATION bin)

# reading sequence set 1
add_executable(tobii_pro_sequence_set1 WIN32 src/tobii_pro_sequence_set1.cpp)

target_include_directories(tobii_pro_sequence_set1 PRIVATE ${TOBII_PRO_SDK_INCLUDE})

target_link_libraries(tobii_pro_sequence_set1 "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.lib" gdi32 user32)

add_custom_command(TARGET tobii_pro_sequence_set1 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.dll" $<TARGET_FILE_DIR:tobii_pro_sequence_set1>)

# reading sequence set 2
add_executable(tobii_pro_sequence_set2 WIN32 src/tobii_pro_sequence_set2.cpp)

target_include_directories(tobii_pro_sequence_set2 PRIVATE ${TOBII_PRO_SDK_INCLUDE})

target_link_libraries(tobii_pro_sequence_set2 "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.lib" gdi32 user32)

add_custom_command(TARGET tobii_pro_sequence_set2 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.dll" $<TARGET_FILE_DIR:tobii_pro_sequence_set2>)

# reading sequence set 3
add_executable(tobii_pro_sequence_set3 WIN32 src/tobii_pro_sequence_set3.cpp)

target_include_directories(tobii_pro_sequence_set3 PRIVATE ${TOBII_PRO_SDK_INCLUDE})

target_link_libraries(tobii_pro_sequence_set3 "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.lib" gdi32 user32)

add_custom_command(TARGET tobii_pro_sequence_set3 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.dll" $<TARGET_FILE_DIR:tobii_pro_sequence_set3>)

# reading sequence set 4
add_executable(tobii_pro_sequence_set4 WIN32 src/tobii_pro_sequence_set4.cpp)

target_include_directories(tobii_pro_sequence_set4 PRIVATE ${TOBII_PRO_SDK_INCLUDE})

target_link_libraries(tobii_pro_sequence_set4 "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.lib" gdi32 user32)

add_custom_command(TARGET tobii_pro_sequence_set4 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.dll" $<TARGET_FILE_DIR:tobii_pro_sequence_set4>)

# reading sequence all (integrated version)
add_executable(tobii_pro_sequence_all WIN32 src/tobii_pro_sequence_all.cpp)

target_include_directories(tobii_pro_sequence_all PRIVATE ${TOBII_PRO_SDK_INCLUDE})

target_link_libraries(tobii_pro_sequence_all "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.lib" gdi32 user32)

add_custom_command(TARGET tobii_pro_sequence_all POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TOBII_PRO_SDK_LIB}/${ARCH}/tobii_research.dll" $<TARGET_FILE_DIR:tobii_pro_sequence_all>)

install(TARGETS tobii_pro_sequence_set1 tobii_pro_sequence_set2 tobii_pro_sequence_set3 tobii_pro_sequence_set4 tobii_pro_sequence_all RUNTIME DESTINATION bin) 