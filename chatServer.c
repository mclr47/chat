#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
//#include <stdatomic.h>
#include <signal.h>
#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
static sig_atomic_t cliCount=0;
static int uid=10;
int i=0;
  
  /*  client */
 typedef struct{
	  struct sockaddr_in addr; /* client remote address*/
          int connfd; //conn files descriptor
	  int uid;   //unique client identifier
	  char name[32];
          }client_t;
client_t *clients[MAX_CLIENTS];
pthread_mutex_t clientsMutex=PTHREAD_MUTEX_INITIALIZER;

static char topic[BUFFER_SZ/2];
pthread_mutex_t topicMutex=PTHREAD_MUTEX_INITIALIZER;

   char * strdup(const char *s)  //for string duplication
     {size_t size=strlen(s)+1;
	     char *p=malloc(size);
	     if(p){memcpy(p,s,size);
	       }
	     return p;
     }
void qAdd(client_t *cl)
	 {pthread_mutex_lock(&clientsMutex);
          for( i=0;i<MAX_CLIENTS;++i)
               { if(!clients[i])
                   {clients[i]=cl;
                     break;
                     }
	       }
	 pthread_mutex_unlock(&clientsMutex);
	 }	  
void qDelete(int uid)
    {pthread_mutex_lock(&clientsMutex);
      for( i=0;i<MAX_CLIENTS;++i)
        { if(clients[i])
		 {if(clients[i]->uid==uid)
			  {clients[i]=NULL;
			   break;
		           }
	          } 		  
	}   		
    pthread_mutex_unlock(&clientsMutex); 
    }		
//**********************************************
//send the message tom all except the sender himself
 void send_m(char *s,int uid)
	  {pthread_mutex_lock(&clientsMutex);
            for( i=0;i<MAX_CLIENTS;++i)
             {if(clients[i])
                    {if(clients[i]->uid!=uid)
                      if(write(clients[i]->connfd,s,strlen(s))<0)
			       {perror("write to descriptor failed\n");
				       break;
			       }
	           }
               }
	    pthread_mutex_unlock(&clientsMutex);

            }
 //*******************************************
  void send_m_all(char *s)
    {pthread_mutex_lock(&clientsMutex);
	for( i=0;i<MAX_CLIENTS;++i)
                {if(clients[i])
		    {if(write(clients[i]->connfd,s,strlen(s))<0)
		      {perror("Write to descriptor failed\n");
			      break;
		    }
	            }
		  }
	 pthread_mutex_unlock(&clientsMutex);
	
     } 
//***************************************************************  
              		    
void send_m_self(const char *s, int connfd)
      {if(write(connfd,s,strlen(s))<0)
	      {perror("writing  to file descriptor failed\n");
	       exit(-1);

	      }
        }
//***********************************************************
// send message to client
void send_m_client(char *s,int uid)
	{ pthread_mutex_lock(&clientsMutex);
		for( i=0; i<MAX_CLIENTS;++i)
                  {if(clients[i])
			  {if(clients[i]->uid==uid)
		            {if(write(clients[i]->connfd,s,strlen(s))<0)
		               {perror("write to the dscriptor failed\n");
		               }
	                    }
                           } 
                   }
     pthread_mutex_unlock(&clientsMutex);
       } 

//*********************************************************
//
//send lists of active clients
 void send_active_c(int connfd)
 {  char s[64];
	 pthread_mutex_lock(&clientsMutex);
	  for(i=0; i<MAX_CLIENTS;++i)
		 {if(clients[i])
			 {sprintf(s,"<<[%d] %s \r\n",clients[i]->uid,clients[i]->name);
 send_m_self(s,connfd);
			 }
		 }
           pthread_mutex_unlock(&clientsMutex);
   }	   
void stripNewLine(char *s)
	
	 {while(*s!='\0')
		 {if(*s=='\r'||*s=='\n')
			 {*s='\0';
	                  }
       s++;
            }
	 }
 //**********************************************************************
void printClientAdress(struct sockaddr_in addr)
{ printf("%d.%d.%d.%d",
		addr.sin_addr.s_addr& 0xff,
		(addr.sin_addr.s_addr&0xff00)>>8,
	        (addr.sin_addr.s_addr&0xff00)>>16,

/*
 *
 *     s_addr is variable that holds the information about the address we agree to accept. So, in this case i put INADDR_ANY because i would like to accept connections from any internet address. This case is used about server example. In a client example i could NOT accept connections from ANY ADDRESS.
 *
 *     ServAddr.sin_addr.s_addr = htonl(INADDR_ANY); */		(addr.sin_addr.s_addr&0xff00)>>24);
}

       	//*******************************************************************

void *handle_client(void *arg)
{ char buff_out[BUFFER_SZ];
  char buff_in[BUFFER_SZ/2];
  int rlen;
  cliCount++;
  client_t *cli=(client_t*)arg;
  printf("<<accept");
  printClientAdress(cli->addr);
  printf("  referenced by %d\n",cli->uid);
  ///
  sprintf(buff_out,"##%s has joined\r\n",cli->name);
  send_m_all(buff_out);
  //
  pthread_mutex_lock(&topicMutex);
  if(strlen(topic))
  {buff_out[0]='\0';
   sprintf(buff_out,"<<topic:%s\r\n",topic);
   send_m_self(buff_out,cli->connfd);
  }
  pthread_mutex_unlock(&topicMutex);
   //
 //
   send_m_self("<<see/help for assistance\r\n",cli->connfd);
   
   //Receive input from the client
    while((rlen=read(cli->connfd,buff_in,sizeof(buff_in)-1))>0)
     {buff_in[rlen]='\0';
        buff_out[0]='\0';
      	stripNewLine(buff_in);

	if(!strlen(buff_in))
	{ continue;}

	/**particular cases***/
   /*	if(buff-in[0]=='/')
	  {char *command , *param;
	   command=strtok(buff_in," ");
	   
	 */
	 snprintf(buff_out, sizeof(buff_out), "[%s] %s\r\n", cli->name, buff_in);
	    send_m(buff_out, cli->uid);
       }
//close the conection
     sprintf(buff_out,"###%s has left\r\n",cli->name);
send_m_all(buff_out);
  close(cli->connfd);
  
  //take client off the queue and yield thread
 qDelete(cli->uid);
 printf("###quit");
 printClientAdress(cli->addr);
  printf("with client user id:%d\n",cli->uid);
  free(cli);
  cliCount--;
  pthread_detach(pthread_self());
  alarm (1);
   return NULL;
     }
int main(int argc, char *argv[])
    {int listenfd=0,connfd=0;
    struct sockaddr_in serverA;
    struct sockaddr_in clientA;
    pthread_t tid;
  /*socket settings*/
    listenfd=socket(AF_INET,SOCK_STREAM,0);
    serverA.sin_family=AF_INET;
    serverA.sin_addr.s_addr=htonl(INADDR_ANY);
    serverA.sin_port=htonl(5000);

    //ignore pipe signals

    signal(SIGPIPE,SIG_IGN);

     //BIND .....CONNECT
     if(bind(listenfd,(struct sockaddr*)&serverA,sizeof(serverA))<0)
	      {perror("Socket binding failed");
		   return EXIT_FAILURE;
	      }
       /* LISTEN  */
  if(listen(listenfd,10)<0)
    {perror("Soccket listening failed");
     return EXIT_FAILURE;
    }
  printf("<<[SERVER STARTED ]>>\n");

    //accept clients
alarm (1);
  while(1)
    {
	 socklen_t clilen=sizeof(clientA);
	 connfd=accept(listenfd,(struct sockaddr*)&clientA,&clilen); 
     if((cliCount+1)==MAX_CLIENTS)
        { printf("max no of clients, reached\n");
	  printf("unable to serve for client :");
	  printClientAdress(clientA);
           close(connfd);	   
         continue;
     	} 
    client_t *cli=(client_t*)malloc(sizeof(client_t));
		  cli->addr=clientA;
	          cli->connfd=connfd;
                  cli->uid=uid++;
                  sprintf(cli->name,"%d",cli->uid);
       qAdd(cli);
       pthread_create(&tid, NULL,&handle_client,(void*)cli);
       sleep(1);
       }
       return 0;
       }
	         	  
         
    
               

 //*******************************************************************



 //*******************************************************************
							   
          	      

 //*******************************************************************
 // //******************************************************************
  //*******************************************************************
  // //*******************************************************************          			    
   //*******************************************************************
   //*******************************************************************
 //*******************************************************************	   
 //*******************************************************************
 //*******************************************************************
 // //*******************************************************************
 //  //*******************************************************************
 //   //*******************************************************************
 //    //*******************************************************************
