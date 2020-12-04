add_compile_flags_target(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources_local(${PROJECT_NAME}
    PRIVATE
        app/BlockEncoder.cpp
        app/check_blocks.cpp
        app/Chunk.cpp
        app/decode_change.cpp
        app/load_all_block_headers.cpp
        app/loadAllBlockHashes.cpp
        app/parse_block.cpp
        app/show_block_changes.cpp
        app/utxo_to_change.cpp
        app/Utxo.cpp
        app/UtxoV2.cpp
        app/Visualizer.cpp
        buv/SocketStream.cpp
        unit/BlockEncoderTest.cpp
        unit/ChunkTest.cpp
        unit/HexTest.cpp
        unit/parallelToSequentialTest.cpp
        unit/UtxoTest.cpp
        unit/VarIntTest.cpp
        util/doctest.cpp
        util/hex.cpp
        util/HttpClient.cpp
        util/Mmap.cpp
        util/nanobench.cpp
        util/parallelToSequential.cpp
)
