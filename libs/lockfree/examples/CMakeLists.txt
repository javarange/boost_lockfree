set(examples
    fifo.cpp
    stack.cpp
    ringbuffer.cpp
)

link_libraries(${Boost_THREAD_LIBRARY})

foreach(ex ${examples})
  string(REPLACE .cpp "" example_name ${ex} )
  add_executable(${example_name} ${ex})
endforeach(ex)
