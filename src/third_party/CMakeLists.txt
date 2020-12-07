# cpp-httplib
target_include_directories(${PROJECT_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib"
)

# doctest
target_include_directories(${PROJECT_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/doctest/doctest"
)

# fmt
target_include_directories(${PROJECT_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/fmt/include"
)
target_sources(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/fmt/src/format.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/fmt/src/os.cc"
)

# font-chef
add_subdirectory(font-chef EXCLUDE_FROM_ALL)
find_package(font-chef)
target_link_libraries(${PROJECT_NAME} PUBLIC font-chef)


# nanobench
target_include_directories(${PROJECT_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/nanobench/src/include"
)

# robin-hood-hashing
target_include_directories(${PROJECT_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/robin-hood-hashing/src/include"
)

# simdjson
target_include_directories(${PROJECT_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/simdjson/singleheader"
)
target_sources(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/simdjson/singleheader/simdjson.cpp"
)
