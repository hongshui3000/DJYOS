################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/eclipseCppIndigo/djysrc/djyos/bsp/board_peripheral/qh_1/nandflash/k9f1208xxx.c 

OBJS += \
./src/bsp/board_peripheral/nandflash/k9f1208xxx.o 

C_DEPS += \
./src/bsp/board_peripheral/nandflash/k9f1208xxx.d 


# Each subdirectory must supply rules for building sources it contributes
src/bsp/board_peripheral/nandflash/k9f1208xxx.o: C:/eclipseCppIndigo/djysrc/djyos/bsp/board_peripheral/qh_1/nandflash/k9f1208xxx.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -Ddebug -I"C:/eclipseCppIndigo/djysrc/examples/qh_1_tiny/src/user/include" -I"C:/eclipseCppIndigo/djysrc/examples/qh_1_tiny/src/user/config" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\arch\arm\cortex_m\cortex_m3\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\stm32\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\board_peripheral\qh_1\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\gui\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\djyfs\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\std_inc" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\vendor_lib/CMSIS/CM3/CoreSupport" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\vendor_lib/stm32" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\vendor_lib/stm32/STM32F10x_StdPeriph_Driver/inc" -O0 -ffunction-sections -fdata-sections -Wall -std=c99 -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


