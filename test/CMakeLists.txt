# add -Wnull-deference for g++ >= 6
# add -Wrestrict g++ >= 7
# add -Wconversion
# add -Wduplicated-cond for g++ >= 6
# add -Wduplicated-branches for g++ >= 7

macro(add_args tgt)
target_compile_options(
  ${tgt}
  PUBLIC
  $<$<COMPILE_LANGUAGE:CXX>:
    -Wall;
    -Wextra;
    -Wpedantic;
    -Wcast-align;
    -Wdisabled-optimization;
    -Winit-self;
    -Wlogical-op;
    -Wmissing-include-dirs;
    -Woverloaded-virtual;
    -Wpointer-arith;
    -Wshadow;
    -Wstrict-aliasing;
    -Wswitch-enum;
    -Wundef;
    -Wvla;
    -Wformat=2;
  >
)
endmacro()

add_executable(test_all test_main.cpp
  test_hwgraph.cpp
  test_graph.cpp
)

add_args(test_all)
target_include_directories(test_all SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)
target_link_libraries(test_all hwgraph)
add_test(NAME test_all COMMAND ./test_all -a)

