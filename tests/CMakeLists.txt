add_executable(astreeApiTests api_tests.cpp)
target_include_directories(astreeApiTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")

foreach(test_case IN ITEMS
        factories
        nullable_factory_children
        child_access_and_indices
        reparenting_and_duplicates
        removal
        parent_access
        set_as_parent
        unique_names
        lifetimes
        walker_single_node
        walker_tree
        walker_rebuild)
    add_test(NAME astree_${test_case} COMMAND astreeApiTests ${test_case})
endforeach()
