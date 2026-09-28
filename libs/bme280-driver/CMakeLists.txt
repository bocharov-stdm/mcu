    
cmake_minimum_required(VERSION 3.15)

project(bme280-driver)

add_library(${PROJECT_NAME} STATIC
	"src/bme280-driver.c"
)

target_include_directories(${PROJECT_NAME} PUBLIC
	"include"
)