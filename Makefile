############################# Makefile ##########################
all: receiver sender
receiver: termio_f.o  receiver.o
	gcc -o receiver receiver.o termio_f.o
        
sender: termio_f.o  sender.o
	gcc -o sender sender.o termio_f.o
        
termio_f.o: termio_f.c
	gcc -o termio_f.o -c termio_f.c
receiver.o: 
	gcc -o receiver.o -c receiver.c
sender.o:
	gcc -o sender.o -c sender.c
	
	
clean:
	rm *.o receiver sender
