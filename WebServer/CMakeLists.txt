set(SOURCES
    Channel.cpp 
    Epoll.cpp 
    EventLoop.cpp 
    EventLoopThread.cpp 
    EventLoopThreadPool.cpp 
    HttpData.cpp 
    Main.cpp 
    Server.cpp 
    Timer.cpp 
    Utils.cpp 
)

include_directories(${PROJECT_SOURCE_DIR}/base)
add_executable(WebServer ${SOURCES})
target_link_libraries(WebServer libserver_base)
add_subdirectory(base)