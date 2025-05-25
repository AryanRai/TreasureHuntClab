################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/objs/GPIO.c \
../Core/Src/objs/door_manager_obj.c \
../Core/Src/objs/door_obj.c \
../Core/Src/objs/gpio_josh.c \
../Core/Src/objs/serial_josh.c \
../Core/Src/objs/servo_driver_obj.c \
../Core/Src/objs/timer_josh.c 

OBJS += \
./Core/Src/objs/GPIO.o \
./Core/Src/objs/door_manager_obj.o \
./Core/Src/objs/door_obj.o \
./Core/Src/objs/gpio_josh.o \
./Core/Src/objs/serial_josh.o \
./Core/Src/objs/servo_driver_obj.o \
./Core/Src/objs/timer_josh.o 

C_DEPS += \
./Core/Src/objs/GPIO.d \
./Core/Src/objs/door_manager_obj.d \
./Core/Src/objs/door_obj.d \
./Core/Src/objs/gpio_josh.d \
./Core/Src/objs/serial_josh.d \
./Core/Src/objs/servo_driver_obj.d \
./Core/Src/objs/timer_josh.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/objs/%.o Core/Src/objs/%.su Core/Src/objs/%.cyclo: ../Core/Src/objs/%.c Core/Src/objs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F303xC -c -I../Core/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F3xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-objs

clean-Core-2f-Src-2f-objs:
	-$(RM) ./Core/Src/objs/GPIO.cyclo ./Core/Src/objs/GPIO.d ./Core/Src/objs/GPIO.o ./Core/Src/objs/GPIO.su ./Core/Src/objs/door_manager_obj.cyclo ./Core/Src/objs/door_manager_obj.d ./Core/Src/objs/door_manager_obj.o ./Core/Src/objs/door_manager_obj.su ./Core/Src/objs/door_obj.cyclo ./Core/Src/objs/door_obj.d ./Core/Src/objs/door_obj.o ./Core/Src/objs/door_obj.su ./Core/Src/objs/gpio_josh.cyclo ./Core/Src/objs/gpio_josh.d ./Core/Src/objs/gpio_josh.o ./Core/Src/objs/gpio_josh.su ./Core/Src/objs/serial_josh.cyclo ./Core/Src/objs/serial_josh.d ./Core/Src/objs/serial_josh.o ./Core/Src/objs/serial_josh.su ./Core/Src/objs/servo_driver_obj.cyclo ./Core/Src/objs/servo_driver_obj.d ./Core/Src/objs/servo_driver_obj.o ./Core/Src/objs/servo_driver_obj.su ./Core/Src/objs/timer_josh.cyclo ./Core/Src/objs/timer_josh.d ./Core/Src/objs/timer_josh.o ./Core/Src/objs/timer_josh.su

.PHONY: clean-Core-2f-Src-2f-objs

