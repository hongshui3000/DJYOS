################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/djysdk/djysrc/djyos/bsp/board_peripheral/qh_1/STMPE811/I2C_STM32.c \
D:/djysdk/djysrc/djyos/bsp/board_peripheral/qh_1/STMPE811/TS_I2C.c \
D:/djysdk/djysrc/djyos/bsp/board_peripheral/qh_1/STMPE811/stmpe811.c 

OBJS += \
./src/bsp/board_peripheral/STMPE811/I2C_STM32.o \
./src/bsp/board_peripheral/STMPE811/TS_I2C.o \
./src/bsp/board_peripheral/STMPE811/stmpe811.o 

C_DEPS += \
./src/bsp/board_peripheral/STMPE811/I2C_STM32.d \
./src/bsp/board_peripheral/STMPE811/TS_I2C.d \
./src/bsp/board_peripheral/STMPE811/stmpe811.d 


# Each subdirectory must supply rules for building sources it contributes
src/bsp/board_peripheral/STMPE811/I2C_STM32.o: D:/djysdk/djysrc/djyos/bsp/board_peripheral/qh_1/STMPE811/I2C_STM32.c
	@echo 'Building file: $<'
	@echo 'Invoking: Sourcery GCC C Compiler'
	arm-none-eabi-gcc -Ddebug -I"D:/djyos_workspace/easy175x/src/user/include" -I"D:/djyos_workspace/easy175x/src/user/config" -I"D:\djysdk\\djysrc\djyos\bsp\arch\arm\cortex_m\cortex_m3\include" -I"D:\djysdk\\djysrc\djyos\bsp\board_peripheral\qh_1\include" -I"D:\djysdk\\djysrc\djyos\djyos\include" -I"D:\djysdk\\djysrc\djyos\djyos\gui\include" -I"D:\djysdk\\djysrc\djyos\djyos\djyfs\include" -I"D:\djysdk\\djysrc\djyos\bsp\cpu_peripheral\std_inc" -I"D:\djysdk\\djysrc\djyos\bsp\vendor_lib/CMSIS/CM3/CoreSupport" -I"D:\djysdk\\djysrc\djyos\bsp\cpu_peripheral\lpc175x\include" -O0 -ffunction-sections -fdata-sections -std=c99 -c -Wa,-adhlns="$@.lst" -Wall -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g2 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/bsp/board_peripheral/STMPE811/TS_I2C.o: D:/djysdk/djysrc/djyos/bsp/board_peripheral/qh_1/STMPE811/TS_I2C.c
	@echo 'Building file: $<'
	@echo 'Invoking: Sourcery GCC C Compiler'
	arm-none-eabi-gcc -Ddebug -I"D:/djyos_workspace/easy175x/src/user/include" -I"D:/djyos_workspace/easy175x/src/user/config" -I"D:\djysdk\\djysrc\djyos\bsp\arch\arm\cortex_m\cortex_m3\include" -I"D:\djysdk\\djysrc\djyos\bsp\board_peripheral\qh_1\include" -I"D:\djysdk\\djysrc\djyos\djyos\include" -I"D:\djysdk\\djysrc\djyos\djyos\gui\include" -I"D:\djysdk\\djysrc\djyos\djyos\djyfs\include" -I"D:\djysdk\\djysrc\djyos\bsp\cpu_peripheral\std_inc" -I"D:\djysdk\\djysrc\djyos\bsp\vendor_lib/CMSIS/CM3/CoreSupport" -I"D:\djysdk\\djysrc\djyos\bsp\cpu_peripheral\lpc175x\include" -O0 -ffunction-sections -fdata-sections -std=c99 -c -Wa,-adhlns="$@.lst" -Wall -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g2 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/bsp/board_peripheral/STMPE811/stmpe811.o: D:/djysdk/djysrc/djyos/bsp/board_peripheral/qh_1/STMPE811/stmpe811.c
	@echo 'Building file: $<'
	@echo 'Invoking: Sourcery GCC C Compiler'
	arm-none-eabi-gcc -Ddebug -I"D:/djyos_workspace/easy175x/src/user/include" -I"D:/djyos_workspace/easy175x/src/user/config" -I"D:\djysdk\\djysrc\djyos\bsp\arch\arm\cortex_m\cortex_m3\include" -I"D:\djysdk\\djysrc\djyos\bsp\board_peripheral\qh_1\include" -I"D:\djysdk\\djysrc\djyos\djyos\include" -I"D:\djysdk\\djysrc\djyos\djyos\gui\include" -I"D:\djysdk\\djysrc\djyos\djyos\djyfs\include" -I"D:\djysdk\\djysrc\djyos\bsp\cpu_peripheral\std_inc" -I"D:\djysdk\\djysrc\djyos\bsp\vendor_lib/CMSIS/CM3/CoreSupport" -I"D:\djysdk\\djysrc\djyos\bsp\cpu_peripheral\lpc175x\include" -O0 -ffunction-sections -fdata-sections -std=c99 -c -Wa,-adhlns="$@.lst" -Wall -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g2 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


