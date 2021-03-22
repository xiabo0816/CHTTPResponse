PROJECT(CHTTPResponse)

CMAKE_MINIMUM_REQUIRED(VERSION 3.14)
SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb -pthread  -lstdc++ -D_GLIBCXX_USE_CXX11_ABI=0")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -std=c++11 -Wall -g2 -ggdb -pthread -lstdc++ -fpermissive -Wno-write-strings")
#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -pthread  -lstdc++ -D_GLIBCXX_USE_CXX11_ABI=0")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -std=c++11 -w -pthread -lstdc++ -fpermissive")

# SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
# SET(THREADS_PREFER_PTHREAD_FLAG TRUE)
# set(CMAKE_CXX_FLAGS "-static ${CMAKE_CXX_FLAGS}")

ADD_SUBDIRECTORY(src/event2)
AUX_SOURCE_DIRECTORY(src DIR_SRCS)
#AUX_SOURCE_DIRECTORY(. DIR_SRCS)
ADD_EXECUTABLE(CHTTPResponse ${DIR_SRCS})

# sudo apt-get install glibc-doc
# sudo yum install libstdc++.x86_64
# sudo apt-get install manpages-posix-dev

find_package(Threads REQUIRED)
TARGET_LINK_LIBRARIES(CHTTPResponse ${CMAKE_THREAD_LIBS_INIT})
TARGET_LINK_LIBRARIES(CHTTPResponse Http) # 向可执行文件hello提供链接库Http是前面提到的变量
#TARGET_LINK_LIBRARIES(BCC40 SRCS)