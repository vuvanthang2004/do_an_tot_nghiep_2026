# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
# yêu cầu phiên bản tối thiểu của cmake là 3.16, nếu cũ hơn thì lỗi

cmake_minimum_required(VERSION 3.16)
"""
#Nạp công cụ build vào project, công cụ bao gồm idf.py, cmake, ninja,gcc,linker,ccache. nếu thiếu dòng này thì sẽ ko build được, 
ko tìm đc thư viện và không biết trình biên dịch ở đâu nên ko biên dịch được
nó quyết định việc build được hay không
"""
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(DO_AN)