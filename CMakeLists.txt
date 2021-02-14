cmake_minimum_required(VERSION 3.7)

# specify cross compilers and tools
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_PYTHON python)

#Uncomment for hardware floating point
#set(FPU_FLAGS "-mfloat-abi=hard -mfpu=fpv4-sp-d16")
#add_definitions(-DARM_MATH_CM4 -DARM_MATH_MATRIX_CHECK -DARM_MATH_ROUNDING -D__FPU_PRESENT=1)

#Uncomment for software floating point
set(FPU_FLAGS "-mfloat-abi=soft")

set(COMMON_FLAGS
    "-mcpu=cortex-m3 ${FPU_FLAGS} -mthumb -mthumb-interwork -ffunction-sections -fdata-sections \
    -g -Os -fno-common -fmessage-length=0 -specs=nosys.specs")

set(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS} -std=c++11")
set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS} -std=gnu99")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,-gc-sections,--print-memory-usage")

PROJECT(stm32f1_bootloader C CXX ASM)
set(CMAKE_CXX_STANDARD 11)
# 显示具体命令
# set(CMAKE_VERBOSE_MAKEFILE ON)

if(${stm32f1_bootloader_SOURCE_DIR} STREQUAL ${stm32f1_bootloader_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()


#add_definitions(-DARM_MATH_CM4 -DARM_MATH_MATRIX_CHECK -DARM_MATH_ROUNDING -D__FPU_PRESENT=1)
add_definitions(-DSTM32F103xE -DCFG_TUSB_MCU=OPT_MCU_STM32F1 -DSTM32F1)

file(GLOB_RECURSE C_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
                            "${CMAKE_CURRENT_SOURCE_DIR}/lib/tinyusb/src/*.c")


add_executable(${PROJECT_NAME}.elf ${C_SRCS})
add_dependencies(${PROJECT_NAME}.elf libopencm3_stm32f1.a)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/libopencm3.cmake)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32F103RCTx_FLASH.ld)

target_include_directories(${PROJECT_NAME}.elf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
                                                      ${CMAKE_CURRENT_SOURCE_DIR}/lib/tinyusb/src
                                                      ${CMAKE_CURRENT_SOURCE_DIR}/lib/libopencm3/include
                                                      ${CMAKE_CURRENT_SOURCE_DIR}/lib/tinyusb/hw/mcu/st/cmsis_device_f1/Include
                                                      ${CMAKE_CURRENT_SOURCE_DIR}/lib/libopencm3/include/libopencmsis
                                                      ${CMAKE_CURRENT_SOURCE_DIR}/lib/cmsis_device_f1/Include)

target_link_directories(${PROJECT_NAME}.elf PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME}.elf opencm3_stm32f1)


set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map -T ${LINKER_SCRIPT}")

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")
