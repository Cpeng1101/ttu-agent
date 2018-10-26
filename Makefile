ttu-agent: ttu-client.o ttu-manage.o ttu-common.o
	gcc -o $@ $^ -lpthread

%.o : %.c
	gcc -c -o $@ $<

clean :
	rm *.o
