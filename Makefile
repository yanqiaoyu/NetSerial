#获取.cpp文件
SrcFiles=$(wildcard *.c)
#使用替换函数获取.o文件
ObjFiles=$(patsubst %.c,%.o,$(SrcFiles))
#生成的可执行文件
all:NetSerial
#目标文件依赖于.o文件
NetSerial:$(ObjFiles)
	gcc -g -o $@ -I ./ $(SrcFiles) -ldl -lpthread
#.o文件依赖于.cpp文件，通配使用，一条就够
%.o:%.c
	gcc -g -c -I ./ $< -ldl -lpthread

.PHONY:clean all

clean:
	rm -f *.o
	rm -f NetSerial