################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/eclipseCppIndigo/djysrc/djyos/djyos/memory/memb.c \
C:/eclipseCppIndigo/djysrc/djyos/djyos/memory/mems.c 

OBJS += \
./src/djyos/memory/memb.o \
./src/djyos/memory/mems.o 

C_DEPS += \
./src/djyos/memory/memb.d \
./src/djyos/memory/mems.d 


# Each subdirectory must supply rules for building sources it contributes
src/djyos/memory/memb.o: C:/eclipseCppIndigo/djysrc/djyos/djyos/memory/memb.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -Ddebug -I"C:/eclipseCppIndigo/djysrc/examples/lpc1225-tiny/src/user/include" -I"C:/eclipseCppIndigo/djysrc/examples/lpc1225-tiny/src/user/config" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\gui\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\djyfs\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\std_inc" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\arch\arm\cortex_m\cortex_m0\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\vendor_lib/CMSIS/CM0/CoreSupport" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\lpc12xx\include" -O0 -Wall -std=c99 -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m0 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/djyos/memory/mems.o: C:/eclipseCppIndigo/djysrc/djyos/djyos/memory/mems.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -Ddebug -I"C:/eclipseCppIndigo/djysrc/examples/lpc1225-tiny/src/user/include" -I"C:/eclipseCppIndigo/djysrc/examples/lpc1225-tiny/src/user/config" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\gui\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\djyfs\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\std_inc" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\arch\arm\cortex_m\cortex_m0\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\vendor_lib/CMSIS/CM0/CoreSupport" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\lpc12xx\include" -O0 -Wall -std=c99 -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m0 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

