################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/gamestate.c \
../Src/led_control.c \
../Src/main.c \
../Src/serial.c \
../Src/syscalls.c \
../Src/sysmem.c \
../Src/timer_module.c 

OBJS += \
./Src/gamestate.o \
./Src/led_control.o \
./Src/main.o \
./Src/serial.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/timer_module.o 

C_DEPS += \
./Src/gamestate.d \
./Src/led_control.d \
./Src/main.d \
./Src/serial.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/timer_module.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32F303VCTx -DSTM32 -DSTM32F3 -DSTM32F3DISCOVERY -c -I"C:/Users/163910/OneDrive - UTS/Documents/GitHub/Send files i am mucking aorund with/Assignment 2/stm32f303-definitions/Core/Inc" -I"C:/Users/163910/OneDrive - UTS/Documents/GitHub/TreasureHuntClab/treasurehunt_game_logic/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/gamestate.cyclo ./Src/gamestate.d ./Src/gamestate.o ./Src/gamestate.su ./Src/led_control.cyclo ./Src/led_control.d ./Src/led_control.o ./Src/led_control.su ./Src/main.cyclo ./Src/main.d ./Src/main.o ./Src/main.su ./Src/serial.cyclo ./Src/serial.d ./Src/serial.o ./Src/serial.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.cyclo ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su ./Src/timer_module.cyclo ./Src/timer_module.d ./Src/timer_module.o ./Src/timer_module.su

.PHONY: clean-Src

