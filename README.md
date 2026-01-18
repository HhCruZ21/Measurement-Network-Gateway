inside src 

gcc -g main.c ring_buffer.c threads.c -o mng -pthread

to run server

ss -ltn | grep 5000

in another terminal to notice the server running

gcc -g client.c -o client : To run the client.c (CLI based testing)



GTK UI execution : 

gcc gtk_client.c net_client.c sample_queue.c \
    -o gtk_client \
    `pkg-config --cflags --libs gtk+-3.0` \
    -pthread
