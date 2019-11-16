all:

     $(CC) -Wall chatServer.c -02 -std=c11 -lthread -o chat/server


debug:
	$(CC) -Wall -g chatServer.c -00 -std=c11 -lpthread -o chatServerDbg

clean: 
	

	$(RM) -rf chatServer chatServerDgb 
    
