cmake_minimum_required(VERSION 3.7)
set(OPENCM3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/libopencm3)
set(DEVICES_DATA ${OPENCM3_DIR}/ld/devices.data)
set(DEVICE STM32F103RC)

add_custom_target(libopencm3_stm32f1.a 
                  make -C ${OPENCM3_DIR} TARGETS=stm32/f1 -j8
                  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/lib/libopencm3/lib/libopencm3_stm32f1.a ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Building libopencm3"
                  USES_TERMINAL)
