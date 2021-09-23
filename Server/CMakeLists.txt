add_executable(Server
        Client.cpp
        main.cpp
        Server.cpp
        )

target_lagom(Server)
target_link_libraries(Server PRIVATE Minecraft)
target_include_directories(Server SYSTEM PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
        )