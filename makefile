all:cs321shell

CC = gcc
OBJS = cs321shellv6.o

cs321shell: $(OBJS)
	$(CC) -o cs321shell $(OBJS)
$(OBJS): cs321shellv6.c
	$(CC) -c cs321shellv6.c
clean:
	-@rm -f $(OBJS)
