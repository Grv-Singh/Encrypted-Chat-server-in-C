#include <errno.h>   
#include <unistd.h>	
#include <malloc.h>		
#include <string.h>		
#include <arpa/inet.h>		
#include <sys/socket.h>		
#include <sys/types.h>		
#include <netinet/in.h>        
#include <resolv.h>			
#include "openssl/ssl.h"	
#include "openssl/err.h"	
#include <stdio.h> 		
#define FAIL    -1		
#define BUFFER 1024		
 
int OpenListener(int port)			
{   int sd;
    struct sockaddr_in addr;			
 
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));				
    addr.sin_family = AF_INET;				
    addr.sin_port = htons(port);			
    addr.sin_addr.s_addr = INADDR_ANY;
    if ( bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 ) 
    {
        perror("can't bind port");				
        abort();						
    }
    if ( listen(sd, 10) != 0 )					
    {
        perror("Can't configure listening port");		
        abort();						
    }
    return sd;
}
 
int isRoot()								
{
    if (getuid() != 0)				
    {
        return 0;
    }
    else
    {
        return 1;								
    }
 
}
SSL_CTX* InitServerCTX(void)						
{   SSL_METHOD *method;
    SSL_CTX *ctx;							
 
    OpenSSL_add_all_algorithms();  					
    SSL_load_error_strings();   					
    method = TLSv1_2_server_method();				  	
    ctx = SSL_CTX_new(method);  					 
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
 
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)			
{
    
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
 
void ShowCerts(SSL* ssl)					
{   X509 *cert;
    char *line;
 
    cert = SSL_get_peer_certificate(ssl); 
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);		
        printf("Server: %s\n", line);					
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("client: %s\n", line);					
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificates.\n");
}
 
void Servlet(SSL* ssl) 
{   char buf[1024];
    int sd, bytes;
    char input[BUFFER]; 	
    pid_t cpid;	
    if ( SSL_accept(ssl) == FAIL )     
    ERR_print_errors_fp(stderr);
    else
    {	
        ShowCerts(ssl);        
	
       cpid=fork();
	if(cpid==0)
	{	
	while(1){
	bytes = SSL_read(ssl, buf, sizeof(buf)); 
      
	 if ( bytes > 0 )
        {	
            buf[bytes] = 0;
            printf("\nMESSAGE FROM SERVER:%s\n", buf);
      
        }  
        else
            ERR_print_errors_fp(stderr);
   } }
else {

while(1){

printf("\nMESSAGE TO CLIENT:");
	 fgets(input, BUFFER, stdin);  
	
        SSL_write(ssl, input, strlen(input)); 
}
	
}		
}
    sd = SSL_get_fd(ssl);       
    SSL_free(ssl);         
    close(sd);          
}
 
int main(int count, char *strings[])			
{   SSL_CTX *ctx;
    int server;
    char *portnum;
   
 
    if(!isRoot())								
    {
        printf("This program must be run as root/sudo user!!");
        exit(0);
    }
    if ( count != 2 )
    {
        printf("Usage: %s <portnum>\n", strings[0]);			
        exit(0);
    }
    SSL_library_init();                                                 
 
    portnum = strings[1];
    ctx = InitServerCTX();        
    LoadCertificates(ctx, "certi.pem", "certi.pem"); 
    server = OpenListener(atoi(portnum));    
    
     struct sockaddr_in addr;						
        socklen_t len = sizeof(addr);
        SSL *ssl;
 	listen(server,5);						
        int client = accept(server, (struct sockaddr*)&addr, &len);  
        printf("Connection: %s:%d\n",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));		
        ssl = SSL_new(ctx);              
        SSL_set_fd(ssl, client);      
	        
Servlet(ssl);        
    
    close(server);          
    SSL_CTX_free(ctx);         
}
