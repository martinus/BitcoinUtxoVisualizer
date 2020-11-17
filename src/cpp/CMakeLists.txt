add_compile_flags_target(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources_local(${PROJECT_NAME}
    PRIVATE
        app/utxo_to_change.cpp
        util/doctest.cpp
        util/nanobench.cpp
        unit/Utxo.cpp
)
