################################################################################
# MRS Version: {"version":"1.8.4","date":"2023/02/15"}
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/app_flash.c \
../src/app_kernal.c \
../src/app_main.c \
../src/app_net.c \
../src/app_param.c \
../src/app_port.c \
../src/app_socket.c \
../src/app_sys.c \
../src/app_update.c 

OBJS += \
./src/app_flash.o \
./src/app_kernal.o \
./src/app_main.o \
./src/app_net.o \
./src/app_param.o \
./src/app_port.o \
./src/app_socket.o \
./src/app_sys.o \
./src/app_update.o 

C_DEPS += \
./src/app_flash.d \
./src/app_kernal.d \
./src/app_main.d \
./src/app_net.d \
./src/app_param.d \
./src/app_port.d \
./src/app_socket.d \
./src/app_sys.d \
./src/app_update.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -DDEBUG=1 -I"../StdPeriphDriver/inc" -I"C:\Users\nimo\Desktop\GS02N_Air780E\GS02N_Air780E_boot\src\inc" -I"../RVMSIS" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

