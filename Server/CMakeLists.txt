add_executable(Server
        Client.cpp
        main.cpp
        Server.cpp

        Scripting/Engine.cpp
        Scripting/Format.cpp
        Scripting/Types.cpp
        )

target_lagom(Server)
target_link_libraries(Server PRIVATE Minecraft lua5.3)
target_include_directories(Server SYSTEM PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
        )