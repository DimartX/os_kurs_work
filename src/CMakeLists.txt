cmake_minimum_required(VERSION 3.15)

project(kursach)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-std=c++17 -lpthread -lzmq")

add_executable(router
  router.cpp
  )

add_executable(storage
  storage.cpp
  )

add_executable(main
  main.cpp
  )
