################################################################################
# MRS Version: {"version":"1.8.4","date":"2023/02/15"}
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Task/aes.c \
../Task/app_atcmd.c \
../Task/app_bleRelay.c \
../Task/app_db.c \
../Task/app_encrypt.c \
../Task/app_gps.c \
../Task/app_instructioncmd.c \
../Task/app_jt808.c \
../Task/app_kernal.c \
../Task/app_mir3da.c \
../Task/app_net.c \
../Task/app_param.c \
../Task/app_port.c \
../Task/app_protocol.c \
../Task/app_server.c \
../Task/app_socket.c \
../Task/app_sys.c \
../Task/app_task.c 

OBJS += \
./Task/aes.o \
./Task/app_atcmd.o \
./Task/app_bleRelay.o \
./Task/app_db.o \
./Task/app_encrypt.o \
./Task/app_gps.o \
./Task/app_instructioncmd.o \
./Task/app_jt808.o \
./Task/app_kernal.o \
./Task/app_mir3da.o \
./Task/app_net.o \
./Task/app_param.o \
./Task/app_port.o \
./Task/app_protocol.o \
./Task/app_server.o \
./Task/app_socket.o \
./Task/app_sys.o \
./Task/app_task.o 

C_DEPS += \
./Task/aes.d \
./Task/app_atcmd.d \
./Task/app_bleRelay.d \
./Task/app_db.d \
./Task/app_encrypt.d \
./Task/app_gps.d \
./Task/app_instructioncmd.d \
./Task/app_jt808.d \
./Task/app_kernal.d \
./Task/app_mir3da.d \
./Task/app_net.d \
./Task/app_param.d \
./Task/app_port.d \
./Task/app_protocol.d \
./Task/app_server.d \
./Task/app_socket.d \
./Task/app_sys.d \
./Task/app_task.d 


# Each subdirectory must supply rules for building sources it contributes
Task/%.o: ../Task/%.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -DDEBUG=2 -DCLK_OSC32K=2 -I"../StdPeriphDriver/inc" -I"R:\CODE\CHcode\CH582\GS02N_Air780E\GS02N_Air780E_app\Task\inc" -I"R:\CODE\CHcode\CH582\GS02N_Air780E\GS02N_Air780E_app\APP\include" -I"R:\CODE\CHcode\CH582\GS02N_Air780E\GS02N_Air780E_app\LIB" -I"R:\CODE\CHcode\CH582\GS02N_Air780E\GS02N_Air780E_app\HAL\include" -I"R:\CODE\CHcode\CH582\GS02N_Air780E\GS02N_Air780E_app\APP" -I"../RVMSIS" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

