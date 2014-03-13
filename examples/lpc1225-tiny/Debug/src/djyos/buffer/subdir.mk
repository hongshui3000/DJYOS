################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/eclipseCppIndigo/djysrc/djyos/djyos/buffer/abc.c \
C:/eclipseCppIndigo/djysrc/djyos/djyos/buffer/line.c \
C:/eclipseCppIndigo/djysrc/djyos/djyos/buffer/ring.c 

OBJS += \
./src/djyos/buffer/abc.o \
./src/djyos/buffer/line.o \
./src/djyos/buffer/ring.o 

C_DEPS += \
./src/djyos/buffer/abc.d \
./src/djyos/buffer/line.d \
./src/djyos/buffer/ring.d 


# Each subdirectory must supply rules for building sources it contributes
src/djyos/buffer/abc.o: C:/eclipseCppIndigo/djysrc/djyos/djyos/buffer/abc.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -Ddebug -I"C:/eclipseCppIndigo/djysrc/examples/lpc1225-tiny/src/user/include" -I"C:/eclipseCppIndigo/djysrc/examples/lpc1225-tiny/src/user/config" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\gui\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\djyfs\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\std_inc" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\arch\arm\cortex_m\cortex_m0\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\vendor_lib/CMSIS/CM0/CoreSupport" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\lpc12xx\include" -O0 -Wall -std=c99 -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m0 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/djyos/buffer/line.o: C:/eclipseCppIndigo/djysrc/djyos/djyos/buffer/line.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -Ddebug -I"C:/eclipseCppIndigo/djysrc/examples/lpc1225-tiny/src/user/include" -I"C:/eclipseCppIndigo/djysrc/examples/lpc1225-tiny/src/user/config" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\gui\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\djyfs\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\std_inc" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\arch\arm\cortex_m\cortex_m0\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\vendor_lib/CMSIS/CM0/CoreSupport" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\lpc12xx\include" -O0 -Wall -std=c99 -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m0 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/djyos/buffer/ring.o: C:/eclipseCppIndigo/djysrc/djyos/djyos/buffer/ring.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -Ddebug -I"C:/eclipseCppIndigo/djysrc/examples/lpc1225-tiny/src/user/include" -I"C:/eclipseCppIndigo/djysrc/examples/lpc1225-tiny/src/user/config" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\gui\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\djyos\djyfs\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\std_inc" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\arch\arm\cortex_m\cortex_m0\include" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\vendor_lib/CMSIS/CM0/CoreSupport" -I"C:\eclipseCppIndigo\\djysrc\djyos\bsp\cpu_peripheral\lpc12xx\include" -O0 -Wall -std=c99 -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m0 -mthumb -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


