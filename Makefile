INC_QUEUE = Queue/queue.c Queue/queue_internal.c

stock: 
	cc stock.c $(INC_QUEUE) -o stock

clean: 
	rm -rf stock
