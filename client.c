//
// Created by shachar on 17/12/18.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define NOT_FOUND -1
#define PATH_DEFAULT "/"
#define PORT_DEFAULT 80
#define USAGE_FORMAT "Usage: client [-p <text>] [-r n < pr1=value1 pr2=value2 …>] <URL>\r\n"
#define HTTP_FLAG "http://"
#define PARAMETER_FLAG "-r"
#define POST_FLAG "-p"

typedef struct Client {
    char* text;
    char* hostName;
    char* port;
    char* path;
    char* parameter;
    char* request ;
}Client;

int findPost(int i, char* str);
int findR(int i, char* str);
void textStr (Client *connectHttp, char* argv[] , int postIndex,int argc );
void parameterStr (Client *connectHttp ,char* argv[] , int rIndex ,int howManyP, int argc);
int findUrl ( int i,char* str);
void breakTheUrl (Client *connectHttp , char* url);
int convertNumOfP(Client *connectHttp,char* argv[], int argc, int i);
void checkFormat (Client *connectHttp,char* str);
void buildRequest (Client *connectHttp ,int postIndex,int rIndex);
void InitializingStruct(Client *connectHttp);
int isNum(char* s);
int countDigit(int num);
void freeObject(Client *connectHttp);
int connectServer(Client *connectHttp);
void printUsage ();
void sendAndReceive(Client* connectHttp);

int main (int argc, char* argv[]) {
    int postIndex = NOT_FOUND;
    int rIndex = NOT_FOUND;
    int howManyP = NOT_FOUND;
    char *url = NULL;
    int temp;

    Client *connectHttp = (Client*) malloc(sizeof(Client));   /**will save my heap char* */
    if (connectHttp == NULL) {
        freeObject(connectHttp);
        exit(1);
    }
    InitializingStruct(connectHttp);
    for (int i = 1; i < argc; i++) {
        temp = findPost(i, argv[i]);

        if (temp != NOT_FOUND && postIndex == NOT_FOUND) //will find it only if it the first 1
        {
            postIndex = temp;
            textStr(connectHttp, argv, postIndex,argc);
            i++;  // pass the place of the text. we dont need to over it again.
            continue;
        }
        temp = findR(i, argv[i]);
        if (temp != NOT_FOUND && rIndex == NOT_FOUND) {
            rIndex = temp;
            howManyP = convertNumOfP(connectHttp, argv, argc, rIndex+1);
            if(howManyP == 0)
            {
                i++;
                rIndex = NOT_FOUND;  // its not mean that i found rIndex yet ( 0 parameters)
                continue;
            }
            parameterStr(connectHttp, argv, rIndex, howManyP,argc);
            i += howManyP + 1;  // pass the place of the parameters
            continue;
        }
        temp = findUrl(i, argv[i]);
        if (temp != NOT_FOUND && url == NULL)
        {
            url = argv[i];
            breakTheUrl(connectHttp, url);
            continue;
        } else {
            printUsage();
            freeObject(connectHttp);
            exit(1);
        }
    }
    if (url == NULL)  // mean that i didnt find url
    {
        printUsage();
        freeObject(connectHttp);
        exit(1);
    }
    buildRequest(connectHttp, postIndex, rIndex);
    sendAndReceive(connectHttp);
    freeObject(connectHttp);
        return 0;
}
/** check the parameters format. should be = in the middle*/

void checkFormat (Client *connectHttp,char* str)
{
    char *tmp;
    int index;

    tmp = strchr(str, '=');
    if (tmp == NULL) {
        printUsage ();
        freeObject(connectHttp);
        exit(1);
    } else {
        index = (int) (tmp - str);
        int length = (int) strlen(str);
        if (index == 0 || index + 1 == length)   // check if the = is not in end or in index 0
        {
            printUsage ();
            freeObject(connectHttp);
            exit(1);
        }
    }
}
/** check if after -r we got a number. if yes, return it as a num, if not exit */

int convertNumOfP(Client *connectHttp,char* argv[] , int argc , int i)
{
    if (i < argc && isNum(argv[i]))
        return atoi(argv[i]);
     else{
        printUsage ();
        freeObject(connectHttp);
        exit(0);
    }
}
/** find the index i got the -p => the post index */

int findPost(int i, char* str)
{
    if (strcmp(str,POST_FLAG) == 0)
        return i;
    return NOT_FOUND;
}

/** find the index i got the -r => the parameters index */
int findR(int i, char* str)
{
    if (strcmp(str,PARAMETER_FLAG) == 0)
        return i;
    return NOT_FOUND;
}

/** save the text that comes along with the -p index */

void textStr (Client *connectHttp, char* argv[] , int postIndex ,int argc)
{
    if (postIndex == argc - 1)  // if -p comes in last index we dont have the text
    {
        printUsage();
        freeObject(connectHttp);
        exit(1);
    }
    connectHttp->text =(char*)malloc( (strlen(argv[postIndex+1]) + 1) * sizeof(char));   //create the arr
    if (connectHttp->text == NULL)
    {
        freeObject(connectHttp);
        exit(1);
    }
    bzero(connectHttp->text,(strlen(argv[postIndex+1]) + 1) * sizeof(char));
    strcpy(connectHttp->text,argv[postIndex+1]);  // copy text to my struct
}
/** check if the char* str is a number   */
int isNum(char* s)
{
    int i = 0;
    while(i < strlen(s))
    {
        if (isdigit(s[i]) == 0)    // check every digit if its num
            return 0;
        i++;
    }
    return 1;
}
/** take all the parameters and concatenate it to 1 char* */

void parameterStr (Client *connectHttp ,char* argv[] , int rIndex ,int howManyP,int argc)
{
    int countSize = 0;
    int pStart = rIndex + 2;
   if(pStart+howManyP > argc) // to many parameters that doesnt exist
   {
       printUsage ();
       freeObject(connectHttp);
       exit(1);
   }

    for (int i = pStart; i < pStart + howManyP  ; i++) // sum the size of the new arr
        countSize += strlen(argv[i]);
    countSize += howManyP +1; // size of & and /0

   connectHttp->parameter = (char*)malloc( countSize * sizeof(char));   //create the arr
    if ( connectHttp->parameter == NULL)
    {
        freeObject(connectHttp);
        exit(1);
    }
        bzero(connectHttp->parameter,countSize * sizeof(char));

    for (int i = pStart; i < pStart + howManyP  ; i++){
        if(i == pStart)
            strcat( connectHttp->parameter, "?");   // start with it
        checkFormat (connectHttp,argv[i]);
        strcat( connectHttp->parameter,argv[i]);
        if(i != pStart + howManyP -1)
            strcat(connectHttp->parameter,"&");  // between every parameter
    }
}

/** find the Url - will try find the http:// */
int findUrl ( int i,char* str)
{
   char * url = strstr(str,HTTP_FLAG);
   if (url != NULL)
       return i;
   return NOT_FOUND;
}
/** take the host-name , port, path and save them in the Struct*/

void breakTheUrl (Client *connectHttp , char* url)
{
    char *begin , *end ;
    int length_Host;
    char* temp = NULL;
   char* text = url;

     begin = strstr(text, "/" );
     begin+=2; // go to the start without //
    char* beginPath = strstr(begin, PATH_DEFAULT );   // to take the path!!!

    if(beginPath)  // i got a path!
    {
        int tempLength = (int)(strlen(begin) - strlen(beginPath));

        temp = (char *) malloc((tempLength + 1) * sizeof(char)); // tmp got the area where the port and the host inside. now need to divide it
        if ( temp == NULL)
        {
            freeObject(connectHttp);
            exit(1);
        }
        bzero(temp, (tempLength + 1) * sizeof(char));
        strncpy(temp, begin, tempLength);
        connectHttp->path = (char*)malloc( (strlen(beginPath)+1 ) * sizeof(char));

        if ( connectHttp->path == NULL)
        {
            freeObject(connectHttp);
            exit(1);
        }
        bzero(connectHttp->path,(strlen(beginPath)+1) * sizeof(char));
        strcpy(connectHttp->path,beginPath);
        end = strchr(temp,':');

        if (end)  // i got the port so lets take it
        {
            char* portB = end;
            portB++;
            connectHttp->port = (char*)malloc( (strlen(portB) + 1 ) * sizeof(char));

            if ( connectHttp->port == NULL)
            {
                freeObject(connectHttp);
                exit(1);
            }
            bzero( connectHttp->port,(strlen(portB)+1) * sizeof(char));
            strcpy(connectHttp->port,portB);

            length_Host = (int)(strlen(temp) - strlen(end)) ;

            connectHttp->hostName = (char*)malloc( (length_Host + 1) * sizeof(char));
            if ( connectHttp->hostName == NULL)
            {
                freeObject(connectHttp);
                exit(1);
            }
            bzero(connectHttp->hostName,(length_Host + 1)* sizeof(char));
            strncpy(connectHttp->hostName, temp, length_Host);
            free(temp);

        } else
            connectHttp->hostName = temp;  // i found out that the temp is my host address.

    } else { // i dont have path so put just " / "
        connectHttp->path = (char *) malloc((strlen(PATH_DEFAULT) + 1) * sizeof(char));
        if ( connectHttp->path == NULL)
        {
            freeObject(connectHttp);
            exit(1);
        }
        bzero(connectHttp->path, (strlen(PATH_DEFAULT) + 1) * sizeof(char));
        strcpy(connectHttp->path, PATH_DEFAULT);

        end = strchr(begin, ':');

        if (end)   // i got a port so lets catch it
        {
            char *portB = end;
            portB++;

            int lengthP = (int) strlen(portB);

            connectHttp->port = (char *) malloc((lengthP + 1) * sizeof(char));
            if ( connectHttp->port == NULL)
            {
                freeObject(connectHttp);
                exit(1);
            }
            bzero(connectHttp->port, (lengthP + 1) * sizeof(char));
            strncpy(connectHttp->port, portB, lengthP);

            int length = (int)(strlen(begin) - strlen(end));
            connectHttp->hostName = (char *) malloc((length + 1) * sizeof(char));
            if ( connectHttp->hostName == NULL)
            {
                freeObject(connectHttp);
                exit(1);
            }
            bzero(connectHttp->hostName, (length + 1) * sizeof(char));
            strncpy(connectHttp->hostName, begin, length);

        } else           /// need to add port = 80;
        {
             int length =(int) strlen(begin);  // if i dont have " / " in the end of the url.
            connectHttp->hostName = (char *) malloc((length + 1) * sizeof(char));
            if ( connectHttp->hostName == NULL)
            {
                freeObject(connectHttp);
                exit(1);
            }
            bzero(connectHttp->hostName, (length + 1) * sizeof(char));
            strncpy(connectHttp->hostName, begin, length);
        }
    }
    if (connectHttp->port != NULL && ((int)strlen(connectHttp->port) == 0 ||isNum(connectHttp->port) == 0  )) // if port is empty or if its not a number
    {
        printUsage ();
        freeObject(connectHttp);
        exit(1);
    }
}

/** free the heap char* if its not NULL mean i need to free it*/
void freeObject(Client *connectHttp)
{
   if (connectHttp->parameter)
       free(connectHttp->parameter);
   if (connectHttp->path)
        free(connectHttp->path);
   if (connectHttp->hostName)
        free(connectHttp->hostName);   // free only if its not null
   if (connectHttp->port)
        free(connectHttp->port);
   if(connectHttp->text)
        free(connectHttp->text);
   if(connectHttp->request)
        free(connectHttp->request);
   if (connectHttp)
        free (connectHttp);
}
/** print the Error format usage.*/

void printUsage ()
{
   printf(USAGE_FORMAT);
}

/** connect and build the socket*/
int connectServer(Client *connectHttp){

    struct sockaddr_in sockaddr;
    struct hostent *hp;

    int port = PORT_DEFAULT;  // if there is no port so connect with this 1

    if(connectHttp->port)
        port = atoi(connectHttp->port);

    int socket_fd = NOT_FOUND;
    if((hp = gethostbyname(connectHttp->hostName)) == NULL){
        herror("gethostbyname\n");
        freeObject(connectHttp);
        exit(1);
    }
    bcopy(hp->h_addr, &sockaddr.sin_addr, hp->h_length);

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    socket_fd = socket(PF_INET, SOCK_STREAM, 0);

    if(socket_fd == -1){
        perror("socket\n");
        freeObject(connectHttp);
        exit(1);
    }
    if(connect(socket_fd, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in)) < 0){
        perror("connect\n");
        freeObject(connectHttp);
        exit(1);
    }
    return socket_fd;
}
/** build the request in the right format that we can send it to the server.*/

void buildRequest (Client *connectHttp,int postIndex,int rIndex)
{
    char * http = " HTTP/1.0";
    char * newLine = "\r\n";
    char * host = "Host: ";
    char * post = "POST ";
    char * get = "GET ";
    char * content ="Content-length:";
    int size = 0;  // will sum the size of my request

  if ( postIndex != -1)
  {
      size += strlen(post);  // for POST
      size+= strlen(newLine) + strlen(content)+countDigit((int)strlen(connectHttp->text))+strlen(newLine) ;
      size += strlen(connectHttp->text)  ;  // size of the text
  }
  else
      size += strlen(get);

    if( rIndex != -1)
      size+= strlen(connectHttp->parameter);     // add the parameter length

  size+= strlen(connectHttp->path) + strlen(http);
  size+= strlen(newLine) +strlen(host) + strlen(connectHttp->hostName);
  size+=strlen(newLine)+ strlen(newLine);
  connectHttp->request =(char*)malloc( (size+1) * sizeof(char));  // will be my request place

  if (connectHttp->request == NULL)
  {
      freeObject(connectHttp);
      exit(1);
  }
    bzero(connectHttp->request,(size+1) * sizeof(char));  // clean the string

    if(postIndex!= -1)
        strcat( connectHttp->request, post);
    else
        strcat( connectHttp->request, get);
    strcat( connectHttp->request, connectHttp->path);

    if(rIndex!= -1)
        strcat( connectHttp->request, connectHttp->parameter);

    strcat( connectHttp->request, http);
    strcat( connectHttp->request, newLine);
    strcat( connectHttp->request, host);
    strcat( connectHttp->request, connectHttp->hostName);

    if(postIndex!= -1)
    {
        strcat( connectHttp->request, newLine);
        strcat( connectHttp->request, content);

        char * textLength = (char*)malloc(sizeof(char)*(strlen(connectHttp->text)));
        if (textLength == NULL)
        {
            freeObject(connectHttp);
            exit(1);
        }
        bzero(textLength,sizeof(char)*(strlen(connectHttp->text)));
        sprintf(textLength, "%d", (int)strlen(connectHttp->text));

        strcat( connectHttp->request, textLength);
        free(textLength);
        strcat( connectHttp->request, newLine);
        strcat( connectHttp->request, newLine);
        strcat( connectHttp->request, connectHttp->text);
    } else {
        strcat(connectHttp->request, newLine);  // need double line
        strcat(connectHttp->request, newLine);
    }
}
/** count how many digit i got in the length of the text after -p.*/
int countDigit(int num)
{
    int count = 0;
    while (num > 0)
    {
        count++;
        num = num /10 ;
    }
    return count;
}
/** Initial my Struct and give all the char* a null pointer*/
void InitializingStruct(Client *connectHttp)
{
    connectHttp->hostName = NULL;
    connectHttp->parameter = NULL;
    connectHttp->path = NULL;   // all will be null then i know what i need to free
    connectHttp->port = NULL;
    connectHttp->text = NULL;
    connectHttp->request = NULL;
}
/** send and recive info from the server, after we connect to the server*/
void sendAndReceive(Client* connectHttp)
{
    char buffer[BUFFER_SIZE];
    printf("HTTP request =\n%s\nLEN = %d\n", connectHttp->request, (int) strlen(connectHttp->request));
    int socket_fd = connectServer(connectHttp);

    if (write(socket_fd, connectHttp->request, strlen(connectHttp->request)) < 0) {
        perror("write\r\n");
        exit(1);
    }
    bzero(buffer, BUFFER_SIZE); // clean it
    int sum = 0, n_bytes = 0;

    while((n_bytes= (int)read(socket_fd,buffer, BUFFER_SIZE-1)) > 0){
        printf("%s",buffer);
        sum += n_bytes;
        bzero(buffer, BUFFER_SIZE);
    }
    if (n_bytes < 0) {
        perror("read\r\n");
        freeObject(connectHttp);
        exit(1);
    }
    printf("\n Total received response bytes: %d\n", sum);
    close(socket_fd);
}