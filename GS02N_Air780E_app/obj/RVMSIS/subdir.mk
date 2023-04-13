################################################################################
# MRS Version: {"version":"1.8.4","date":"2023/02/15"}
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../RVMSIS/core_riscv.c 

OBJS += \
./RVMSIS/core_riscv.o 

C_DEPS += \
./RVMSIS/core_riscv.d 


# Each subdirectory must supply rules for building sources it contributes
RVMSIS/%.o: ../RVMSIS/%.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -DDEBUG=2 -DCLK_OSC32K=2 -I"../StdPeriphDriver/inc" -I"C:\Users\nimo\Desktop\GS02N_Air780E\GS02N_Air780E_app\Task\inc" -I"C:\Users\nimo\Desktop\GS02N_Air780E\GS02N_Air780E_app\APP\include" -I"C:\Users\nimo\Desktop\GS02N_Air780E\GS02N_Air780E_app\LIB" -I"C:\Users\nimo\Desktop\GS02N_Air780E\GS02N_Air780E_app\HAL\include" -I"C:\Users\nimo\Desktop\GS02N_Air780E\GS02N_Air780E_app\APP" -I"../RVMSIS" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

