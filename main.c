#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>


#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define CREATE_FLAGS (O_WRONLY | O_TRUNC | O_CREAT )
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define CREATE_APPENDFLAGS (O_WRONLY | O_APPEND | O_CREAT )
 
//this struct help to hold background process information 
typedef struct process
{
    char processName[MAX_LINE];
    int  p_id;
    int  pcount;
    struct process *next; 
}process;

//this struct help to hold bookmark process information 
typedef struct bmp 
{
    char name[MAX_LINE];
    struct bmp *next;
}bmp;

// Global variables
int count = 0;//to count background process number 
int bmCount = 0;//to count bookmark process number 
int isForeground = 0;// to understand is there any foreground process
int fPid = 0;//to hold curent foreground pid
int argCount = 0;// to understand number of argument

// Declare struct pointer
typedef struct process* processPtr;
typedef struct process** processPtrPtr;
typedef struct bmp* bmpPtr;
typedef struct bmp** bmpPtrPtr;

// Global linked list head pointer
bmpPtr bmheader = NULL;// to hold bookmark linked list 
processPtr header = NULL;// to hold running process linked list 
processPtr doneHeader = NULL;// to hold finished process linked list

int insert(char args[],int p_id,int count,processPtrPtr header);

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */
void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
        
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);  

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
    exit(-1);           /* terminate with error code of -1 */
    }

    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
        case ' ':
        case '\t' :               /* argument separators */
        if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
            ct++;
        }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
        start = -1;
        break;

            case '\n':                 /* should be the final char examined */
        if (start != -1){
                    args[ct] = &inputBuffer[start];     
            ct++;
        }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
        break;

        default :             /* some other character */
        if (start == -1)
            start = i;
                if (inputBuffer[i] == '&'){
            *background  = 1;
                    inputBuffer[i-1] = '\0';
        }
    } /* end of switch */
     }    /* end of for */
     args[ct] = NULL; /* just in case the input line was > 80 */
    argCount = ct;
    //for (i = 0; i <= ct; i++)
        //printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */

//This method finds the path and with execv function executes it.
void pathAndExec( char* args[],int background){
    if(background == 1){
        args[argCount-1] = NULL;
    }
    char exec[50];
    char *path = getenv("PATH");
    if(strcmp(args[0],"firefox")==0){
        char binpath[50] = "/bin/";
        strcpy(exec,binpath);
        strcat(exec,args[0]);
        execv(exec,args);
    }
    char *ptr = strtok(path, ":");
    while(ptr != NULL)
	{
        strcpy(exec,ptr);
		ptr = strtok(NULL, ":");
        strcat(exec,"/");
        strcat(exec,args[0]);
        execv(exec,args);
    }

}

// This method create child process and control is process background or not
void mainfunc( char* args[],int background){
    int child_pid = fork();//creating child process
    if(child_pid == -1){
        fprintf(stderr,"Fork failed");       
        return;
    }else if(child_pid == 0){
            printf("");       
            pathAndExec(args,background);
    }else{
        if(background == 0){
            isForeground = 1;
            fPid = child_pid;
            wait(&child_pid);//if process is foreground we use wait method to wait to finish child process
            isForeground = 0;
            fPid = 0; 
        }
        else{
            if(insert(args[0],child_pid,++count,&header)== 0)//insert the process to running linked list
            printf("insertion failed");
        }
    }
}

//Basic Linked list insert method for running linked list
int insert(char args[],int p_id,int count,processPtrPtr head){ 
    processPtr insertingProcess=(process*)malloc(sizeof(process));
    strcpy(insertingProcess->processName,args);
    insertingProcess->p_id = p_id;
    insertingProcess->pcount = count;
    insertingProcess->next = NULL;
    
    if(*head == NULL){
        *head = insertingProcess;
        fflush(stdout);
        return 1;
    }else{
        processPtr temp = *head;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = insertingProcess;
        return 1;
    }

    return 0;

}


//Basic Linked list delete method for running linked list
void delete(int c,processPtrPtr header,int flag){
    process *temp = *header, *prev;

     // If head node itself holds the key to be deleted 
    if (temp != NULL && temp->pcount == c) 
    { 
        if(temp->next != NULL)
        *header = temp->next;  
        else
        {
            *header = NULL;
        }
         // Changed head 
        free(temp); 
        return; 
    }
  
    while (temp != NULL && temp->pcount != c) 
    { 
        prev = temp; 
        temp = temp->next; 
    } 
  
    if (temp == NULL) return; 
    prev->next = temp->next; 
    process *temp2 = temp;
    free(temp); 
}

//Basic Linked list insert method for bookmark linked list
int insertbm(char args[],bmpPtrPtr head){
    
    bmpPtr insertingBmp=(bmp*)malloc(sizeof(bmp));
    strcpy(insertingBmp->name,args);
    insertingBmp->next = NULL;
    if(*head == NULL){
        *head = insertingBmp;
        return 1;
    }else{
        bmpPtr temp = *head;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = insertingBmp;
        return 1;
    }
    return 0;
}

//Basic Linked list delete method for bookmark linked list
void deletebm(int c,bmpPtrPtr header){
    bmp *temp = *header, *prev;

     // If head node itself holds the key to be deleted 
    if (temp != NULL && c==0) 
    { 
        if(temp->next != NULL)
        *header = temp->next;  
        else
        {
            *header = NULL;
        }
        free(temp); 
        return; 
    }
    int i;
    for(i = 0;i<c;i++) 
    {
        if(temp != NULL){
        prev = temp; 
        temp = temp->next;
        } 
    }
  
    if (temp == NULL) return; 
    prev->next = temp->next; 
    bmp *temp2 = temp;
    free(temp); 
}

//This method travers running  linked list and control process status 
void finished(){
   process *temp = header;
    while (temp != NULL){
        int return_pid = waitpid(temp->p_id, NULL, WNOHANG);// we use waitpid method to control process status.
        if (return_pid == -1) {
            /* error */
        } else if (return_pid == 0) {//if waitpid return 0 it means child is still running 
        } else if (return_pid == temp->p_id) {//if waitpid returns the same pid with curent proces pid it means child process finished
            insert(temp->processName, temp->p_id,temp->pcount,&doneHeader);//insert this process to finish linked list 
            delete(temp->pcount,&header,0);//delete this process from running linked list
        }
        temp=temp->next;
    }
}
//This method checks if there are a foreground process running
void funcZ(){
    if(isForeground == 1){
        kill(fPid,0);
        kill(fPid,SIGKILL);//Terminates if there are foreground process with child
        waitpid(-fPid, NULL, WNOHANG);
        
        isForeground = 0;
        fPid = 0;
    }else{printf("\nmyshell:");fflush(stdout);}
}

//This method print running and finished linked list to screen
void ps_all (){
    finished();
    if(header == NULL && doneHeader == NULL){
        }
    else{
        process *temp = header;
        if(temp == NULL)
            printf("\n NOTHING RUNS\n ");
        else
            printf("\n RUNNING \n");
        while(temp!= NULL){
            if(temp->p_id != 0){
                printf("\n [%d] %s (Pid = %d)\n",temp->pcount,temp->processName,temp->p_id);
                fflush(stdout);
            }
            temp = temp->next;
        }
        temp = doneHeader;
        printf("\n FINISHED \n");
        while(temp != NULL){
            if(temp->p_id != 0){
                printf("\n [%d] %s (Pid = %d)\n",temp->pcount,temp->processName,temp->p_id);
                fflush(stdout);
            }
            temp = temp->next;
        }
    }
}

//This method search the given string to file which locate in current directory
void search(char *arr[]){
    
    char str[250]="grep -n ";//Grep command do same job what we want to do search method
    if(strcmp(arr[1],"-r" ) == 0){//If user use search -r tag it means search file recursivle in current directory
       
        char *formats[4]={"*.c*","*.C*","*.h*","*.H* "};
        strcat(str,arr[1]);
        strcat(str," ");
        int l = strlen(arr[2]);
        char exec[l-1];
        char z[25] ="";
        strcpy(z,arr[2]);
        int i = 0;
        for(i = 1; i<l-1;i++){
            exec[i-1] = z[i];
        }
        exec[l-2] = '\0';
        strcat(str,exec);
        int ii= 0;
        
        for(ii; ii< 4 ;ii++){
            strcat(str," --include=");
            strcat(str,formats[ii]);
        }
    }
    else{
        int l = strlen(arr[1]);
        char exec[l-1];
        char z[25] ="";
        strcpy(z,arr[1]);
        int i = 0;
        for(i = 1; i<l-1;i++){
            exec[i-1] = z[i];
        }
        exec[l-2] = '\0';
        strcat(str,exec);
        strcat(str," *.c* *.C* *.h* *.H*");
    }
    system(str);//system method help to execute command in c 
}

//This method execute commmand which located bookmark linked list if user use bookmark -i tag
void bookmark(char str[]){
    int a = atoi(str);//atoi convert string to int and this a variable determine which command will execute
        if(bmheader == NULL ){
            }
        else{
            bmp *temp = bmheader ;
            int i;
            for(i = 0; i< a ; i++)//iterate linked list to find which command will execute
                    temp = temp->next;
            if(temp!=NULL){
                int l = strlen(temp->name);
                char exec[l-1];
                char z[25] ="";
                strcpy(z,temp->name);
                int i = 0;
                for(i = 1; i<l-1;i++){
                    exec[i-1] = z[i];
                }
                exec[l-2] = '\0';
                system(exec);//system method help to execute command in c 
            }
        }
}

//This method create child process after that will execute I/O redirection command according to a parameters
void met (int a ,char* args[]){

    pid_t pid = fork(); //call fork function
    int fileInput = 0,i = 0; //create int for input and output
    int fileOutput = 0;
    char *newArgs[argCount]; 
    char pathPro [100] = "/bin/"; //for path 
    if(pid == 0){
        switch (a)
        {   
        case 1: 
            fileOutput = open(args[argCount - 1], CREATE_FLAGS, CREATE_MODE);//open output file file.out
            if (fileOutput == -1) { //check error
                fprintf(stderr, "Failed to open file");
                return;
            }
            if (dup2(fileOutput, STDOUT_FILENO) == -1) { //dup2 function write the output to the created file
                fprintf(stderr, "Failed to redirect standard output");
                return;
            }
            close(fileOutput); //close file
            
            //execution of the given command
            strcat(pathPro, args[1]); 
            for(i = 0 ; i< argCount-3;i++){
                newArgs[i] = args[i+1];
            }
            
            newArgs[argCount-3] = NULL;

            if (execv(pathPro, newArgs) == -1) {
                fprintf(stderr, "Child process error\n");
            }

            break;
        case 2: 
            fileOutput = open(args[argCount-1],CREATE_APPENDFLAGS,CREATE_MODE);
            if(fileOutput == -1){ //check error
                fprintf(stderr, "Failed to open file");
                return;
            }
            if(dup2(fileOutput,STDOUT_FILENO) == -1){ //dup2 function write the output to the created file
                fprintf(stderr, "Failed to redirect standard output");
                return;
            }
            //close file
            close(fileOutput);
            //execution of the given command
            strcat(pathPro, args[1]);
            for(i = 0 ; i< argCount-3;i++){
                newArgs[i] = args[i+1];
            }
            
            newArgs[argCount-3] = NULL;

            if (execv(pathPro,newArgs) == -1){
                fprintf(stderr, "Child does not work ");
            }
            break;
        case 3: 
            if ((fileInput = open(args[argCount - 1], O_RDONLY)) < 0) {
                fprintf(stderr, "Couldn't open input file\n");
                return;
            }
            dup2(fileInput, STDIN_FILENO);//with dup2 function it executes the input files command
            break;
        case 4: 
            
            if ((fileOutput = open(args[argCount - 1], O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
                fprintf(stderr, "Couldn't open output file\n");
                return;
            }
            
            dup2(fileOutput, STDERR_FILENO);//with the dup2 function it prints the standart error(if there any) to output file
			//execution of the given command
            strcat(pathPro, args[1]);
            for(i = 0 ; i< argCount-3;i++){
                newArgs[i] = args[i+1];
            }
            
            newArgs[argCount-3] = NULL;

            if (execv(pathPro, newArgs) == -1) {
                fprintf(stderr, "Child process error\n");
            }
            close(fileOutput);
            break;
        case 5: 
            fileInput = open(args[argCount - 3], O_RDWR, CREATE_MODE);
            fileOutput = open(args[argCount - 1], CREATE_FLAGS, CREATE_MODE);
           

            if(fileInput == -1){
                fprintf(stderr,"Failed to open file");
                return;
            }
            if(dup2(fileInput, STDIN_FILENO) == -1) {  //gets the commands in the input file 
                fprintf(stderr,"Failed to redirect standard input");
                return;
            }
            close(fileInput);
            
            if (fileOutput == -1) {
                fprintf(stderr,"Creating output file error");
                return;
            }
            if(dup2(fileOutput,STDOUT_FILENO) == -1){//prints the given commands to the output file
                fprintf(stderr,"Failed to redirect standard output");
                return;
            }
            close(fileOutput);
            
            strcat(pathPro,args[0]);
            args[argCount - 4] = NULL;
            execv(pathPro, args);//execution of the command
            
            break;

        }
    }else {
        wait(&pid);
    }


}


int main(void)
{
    
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */

    struct sigaction act;
    act.sa_handler = funcZ;
    act.sa_flags = SA_RESTART;
    if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGTSTP, &act, NULL) == -1)) {
        fprintf(stderr, "Failed to set SIGTSTP handler\n");
        return 1;
    }
    while (1){
        finished();
        background = 0;
        printf("myshell: ");
        fflush(stdout);
        
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);
        
        //we control the argCount to I/O redirection command is valid or not
        
        if(strcmp(args[0],"ps_all") == 0){//if args[0] equals to "ps-all", we call ps_all()
               ps_all();
        }else if (strcmp(args[0],"search") == 0){//if args[0] equals to "search", we call search()
            search(args);
        }else if (strcmp(args[0],"bookmark") == 0){//if args[0] equals to "bookmark", we make four different operations
            if(strcmp(args[1],"-l") == 0){//if tag equals to "-l", we print bookmark linked list to screen 
               if(bmheader == NULL ){
                printf("\n NOTHING\n ");
                }
                else{
                    bmp *temp = bmheader;
                    int i;
                    for (i = 0 ; i<bmCount+1 ;i++){
                        if(temp != NULL){
                        printf("\n [%d] %s\n",i,temp->name);
                        fflush(stdout);
                        temp = temp->next;
                            }
                        }
                }
            }
            else if (strcmp(args[1],"-i") == 0){//if tag equals to "-i", we call bookmark()
                bookmark(args[2]);
            }
            else if (strcmp(args[1],"-d") == 0){//if tag equals to "-d", we call deletebm() which delete process to bookmark linked list 
                bmCount--;
                int a = atoi(args[2]);
                deletebm(a,&bmheader);        
            }
            else{//if bookmark tag doesn't equal anything, we call insertbm() which insert process to bookmark linked list 
                int i=1;
                char str[250] ="";
                while(args[i] != NULL){
                    strcat(str,args[i]);
                    if(args[i+1] != NULL)
                        strcat(str," ");
                    i++;
                }
                insertbm(str,&bmheader);
                bmCount++;
            }
        }else if (strcmp(args[0],"exit") == 0){//if args[0] equals to "exit", we check runing linked list head to understand is there any background process
            if(header == NULL)
            exit(1);//Terminate the code
            else//If we have background process which is still running we will wait to finis background process
            {   
                printf("There are background processes still running.\n");
            }
            
        }
        else{//if args[0] doesn't equals to anything we call mainfunc();
            if(argCount >= 3){
                if(strcmp(args[argCount-2], ">") == 0){
                    if(argCount >=4){
                        if(strcmp(args[argCount-4], "<") == 0){
                            met(5,args);
                        }else met(1,args);
                    }else met(1,args);
                }else if(strcmp(args[argCount-2], ">>") == 0){
                    met(2,args);
                }else if(strcmp(args[argCount-2], "<") == 0){
                    met(3,args);
                }else if(strcmp(args[argCount-2], "2>") == 0){
                    met(4,args);
                }
            }else 
                mainfunc(args,background);
        }
        finished();//we check running background linked list every end of the iteration to check which process finish or not 
    }
}
