file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Net/Packets/Handshake/Serverbound)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Net/Packets/Status/Clientbound)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Net/Packets/Status/Serverbound)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Net/Packets/Login/Clientbound)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Net/Packets/Login/Serverbound)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Net/Packets/Play/Clientbound)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Net/Packets/Play/Serverbound)

function(compile_packet_definition source)
    cmake_path(GET source PARENT_PATH side_full)
    cmake_path(GET side_full PARENT_PATH state_full)

    cmake_path(GET side_full FILENAME side)
    cmake_path(GET state_full FILENAME state)

    cmake_path(GET source STEM name)

    add_custom_command(
            OUTPUT ${state}/${side}/${name}.h
            COMMAND Serializer/Serializer ${CMAKE_CURRENT_SOURCE_DIR}/Net/Packets/${state}/${side}/${name}.json > ${CMAKE_CURRENT_BINARY_DIR}/Net/Packets/${state}/${side}/${name}.h
            VERBATIM
            DEPENDS Serializer
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
endfunction()

file(GLOB_RECURSE packet_definitions ${CMAKE_CURRENT_SOURCE_DIR}/Net/Packets/*.json)
foreach (file ${packet_definitions})
    compile_packet_definition(${file})
endforeach ()

add_library(Minecraft SHARED
        Chat/Component.cpp

        Net/Packets/Status/Clientbound/Response.cpp

        Handshake/Serverbound/Handshake.h

        Status/Serverbound/Ping.h
        Status/Clientbound/Pong.h

        Login/Serverbound/LoginStart.h
        Login/Clientbound/LoginSuccess.h
        Login/Clientbound/Disconnect.h

        Play/Clientbound/ChatMessage.h
        Play/Clientbound/PlayerListHeaderAndFooter.h
        )

target_include_directories(Minecraft SYSTEM PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
        )

target_lagom(Minecraft)
target_link_libraries(Minecraft PRIVATE LagomGfx)
