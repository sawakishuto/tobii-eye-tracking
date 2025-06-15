cmake_minimum_required(VERSION 3.10)
project(TobiiEyeTracking)

# C++17標準を使用
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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