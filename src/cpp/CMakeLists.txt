add_compile_flags_target(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources_local(${PROJECT_NAME}
    PRIVATE
        util/HttpClient.cpp # put first because it is soooo slow

        app/BlockEncoder.cpp
        app/Cfg.cpp
        app/check_blocks.cpp
        app/Chunk.cpp
        app/decode_change.cpp
        app/fetchAllBlockHashes.cpp
        app/find_distant_color.cpp
        app/Hud.cpp
        app/load_all_block_headers.cpp
        app/parse_block.cpp
        app/show_block_changes.cpp
        app/utxo_to_change.cpp
        app/Utxo.cpp
        app/Visualizer.cpp
        buv/SocketStream.cpp
        unit/BlockEncoderTest.cpp
        unit/ChunkTest.cpp
        unit/HexTest.cpp
        unit/OpenCVTest.cpp
        unit/parallelToSequentialTest.cpp
        unit/VarIntTest.cpp
        util/args.cpp
        util/doctest.cpp
        util/hex.cpp
        util/kbhit.cpp
        util/Mmap.cpp
        util/nanobench.cpp
        util/parallelToSequential.cpp
        util/rss.cpp
)
