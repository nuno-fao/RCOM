############################# Makefile ##########################
all: receiver sender
receiver: utils.o  receiver.o
	gcc -o receiver receiver.o utils.o
        
sender: utils.o  sender.o
	gcc -o sender sender.o utils.o
        
utils.o: utils.c
	gcc -o utils.o -c utils.c
receiver.o: 
	gcc -o receiver.o -c receiver.c
sender.o:
	gcc -o sender.o -c sender.c
	
	
clean:
	rm *.o receiver sender
