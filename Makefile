INC_QUEUE = Queue/queue.c Queue/queue_internal.c

stock:	stock.o
	cc *.o -o stock

stock.o: 
	cc -c stock.c $(INC_QUEUE) 

clean: 
	rm -rf stock *.o
