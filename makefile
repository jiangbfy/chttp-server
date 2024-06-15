CROSS_COMPILE	?= #arm-linux-gnueabihf-
TARGET			?= server

CC				:= $(CROSS_COMPILE)gcc

LDFLAGS			:= -lm -lpthread -ldl

INCDIRS			:= include \
					include/http \
					include/server \
					include/pool \
					include/lib \
					include/log

SRCDIRS			:= src \
					src/http \
					src/server \
					src/pool \
					src/lib \
					src/log
					

INCLUDE			:= $(patsubst %, -I %, $(INCDIRS))

CFILES			:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))

CFILENDIR		:= $(notdir  $(CFILES))

COBJS			:= $(patsubst %, obj/%, $(CFILENDIR:.c=.o))
OBJS			:= $(COBJS)

VPATH			:= $(SRCDIRS)

.PHONY: clean
	
$(TARGET) : $(OBJS)
	$(CC) $(COBJS) $(LDFLAGS) -o $(TARGET)

$(COBJS) : obj/%.o : %.c
	$(CC) -Wall -c -O2 $(INCLUDE) -c $< -o $@
	
clean:
	rm -rf $(TARGET) $(COBJS)
