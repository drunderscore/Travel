add_executable(Serializer
        main.cpp
        )

target_include_directories(Serializer SYSTEM PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
        )

target_lagom(Serializer)
