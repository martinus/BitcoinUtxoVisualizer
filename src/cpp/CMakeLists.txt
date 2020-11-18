add_compile_flags_target(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources_local(${PROJECT_NAME}
    PRIVATE
        app/BlockSerializer.cpp
        app/load_all_block_headers.cpp
        app/parse_block.cpp
        app/utxo_to_change.cpp
        app/Utxo.cpp
        unit/Utxo.cpp
        util/doctest.cpp
        util/HttpClient.cpp
        util/Mmap.cpp
        util/nanobench.cpp
)
