#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>

#include <signal.h>

const char* sig_name(int sig) {
    switch(sig) {
        case SIGABRT: return "SIGABRT";
        case SIGFPE:  return "SIGFPE";
        case SIGILL:  return "SIGILL";
        case SIGINT:  return "SIGINT";
        case SIGKILL: return "SIGKILL";
        case SIGQUIT: return "SIGQUIT";
        case SIGSEGV: return "SIGSEGV";
        case SIGTERM: return "SIGTERM";
        case SIGTRAP: return "SIGTRAP";
        case SIGSTOP: return "SIGSTOP";
        case SIGHUP:  return "SIGHUP";
        case SIGALRM: return "SIGALRM";
        case SIGBUS:  return "SIGBUS";
        case SIGPIPE: return "SIGPIPE";
        default: return strsignal(sig);;
    }
}

int main(int argc, char *argv[]){
	pid_t pid;
	int status;

    if (argc != 2) {
        printf("Usage: %s <test_program_name>\n", argv[0]);
        exit(1);
    }

	printf("Process start to fork\n");

	/* fork a child process */
	pid = fork();

	if (pid < 0) {
        /* fork failed */
        perror("Fork failed");
        exit(1);
    }

	/* execute test program */ 
else if (pid == 0) {
        /* child process */
        printf("I'm the Child Process, my pid = %d\n", getpid());
        printf("Child process start to execute test program:\n");
        /* execute test program */
        execl(argv[1], argv[1], NULL);
        
        /* if execl returns, it means it failed */
        perror("execl failed");
        exit(1);
    }	

	else {
        /* parent process */
        printf("I'm the Parent Process, my pid = %d\n", getpid());
        
        /* wait for child process terminates */
        waitpid(pid, &status, WUNTRACED);
        
        printf("Parent process receives SIGCHLD signal\n");
        
        /* check child process' termination status */
        if (WIFEXITED(status)) {
            printf("Normal termination with EXIT STATUS = %d\n", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            printf("child process get %s signal\n", sig_name(sig));
        }
        else if (WIFSTOPPED(status)) {
            int sig = WSTOPSIG(status);
            printf("child process get %s signal\n", sig_name(sig));
        }
        else {
            printf("Child process terminated abnormally\n");
        }
    }
    
    return 0;
}