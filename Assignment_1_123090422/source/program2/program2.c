#include <linux/module.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/jiffies.h>
#include <linux/kmod.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");

struct wait_opts {
	enum pid_type wo_type;
	int wo_flags;
	struct pid *wo_pid;

	struct waitid_info *wo_info;
	int __user wo_stat;
	struct rusage *wo_rusage;

	wait_queue_entry_t child_wait;
	int notask_error;
};

extern struct filename *getname_kernel(const char *filename);
extern pid_t kernel_clone(struct kernel_clone_args *args);
extern int do_execve(struct filename *filename,
		     const char __user *const __user *__argv,
		     const char __user *const __user *__envp);
extern long do_wait(struct wait_opts *wo);

int my_fork(void *argc);

int my_exec(void);

void my_wait(pid_t pid);

int wifexited(int status);
int wexitstatus(int status);
int wifsignaled(int status);
int wtermsig(int status);
int wifstopped(int status);
int wstopsig(int status);

int wifexited(int status)
{ 
	return (((status)&0x7f) == 0);
}

int wexitstatus(int status)
{ 
	return (((status)&0xff00) >> 8);
}

int wifsignaled(int status)
{ 
	return (((signed char)(((status)&0x7f) + 1) >> 1) > 0);
}

int wtermsig(int status)
{
	return ((status)&0x7f);
}

int wifstopped(int status)
{ 
	return (((status)&0xff) == 0x7f);
}

int wstopsig(int status)
{
	return (((status) & (0xff00) >> 8));
}

//implement fork function
int my_fork(void *argc)
{
	pid_t parent_pid = current->pid;
	pid_t child_pid;

	//set default sigaction for current process
	int i;

	struct kernel_clone_args args = { .flags = SIGCHLD,
					   .exit_signal = SIGCHLD,
					   .child_tid = NULL,
					   .parent_tid = NULL,
					   .stack = (unsigned long)&my_exec,
					   .stack_size = 0,
					   .tls = 0 };

	struct k_sigaction *k_action = &current->sighand->action[0];
	for (i = 0; i < _NSIG; i++) {
		k_action->sa.sa_handler = SIG_DFL;
		k_action->sa.sa_flags = 0;
		k_action->sa.sa_restorer = NULL;
		sigemptyset(&k_action->sa.sa_mask);
		k_action++;
	}

	/* fork a process using kernel_clone or kernel_thread */
	child_pid = kernel_clone(&args);
	printk("[program2] : The child process has pid = %d\n", child_pid);

	/* wait until child process terminates */
	printk("[program2] : This is the parent process, pid = %d\n",
	       parent_pid);
	printk("[program2] : child process\n");
	my_wait(child_pid);

	return 0;
}

int my_exec(void)
{
	char path[] = "/tmp/test";
	struct filename *file_pointer = getname_kernel(path); 

	do_execve(file_pointer, NULL, NULL);

	return 0;
}

void my_wait(pid_t pid)
{
	int status;
	int a;
	struct wait_opts wopt;
	struct pid *wo_pid_ptr = NULL;
	enum pid_type type;
	type = PIDTYPE_PID;
	wo_pid_ptr = find_get_pid(pid);

	wopt.wo_type = type;
	wopt.wo_pid = wo_pid_ptr;
	wopt.wo_flags = WUNTRACED | WEXITED;
	wopt.wo_info = NULL;
	wopt.wo_stat = status;
	wopt.wo_rusage = NULL;

	a = do_wait(&wopt);

	switch ((wopt.wo_stat) & 0x7f) {
	case 1: {
		printk("[program2] : get SIGHUP signal\n");
		printk("[program2] : child process hangs up controlling terminal or process\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}
	case 2: {
		printk("[program2] : get SIGINT signal\n");
		printk("[program2] : child process has interrupt from keyboard\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}	
	case 3: {
		printk("[program2] : get SIGQUIT signal\n");
		printk("[program2] : child process quits from keyboard\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}
	case 4: {
		printk("[program2] : get SIGILL signal\n");
		printk("[program2] : child process has illegal instruction\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}	
	case 5: {
		printk("[program2] : get SIGTRAP signal\n");
		printk("[program2] : child process has breakpoint for debugging\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}
	case 6: {
		printk("[program2] : get SIGABRT signal\n");
		printk("[program2] : child process is aborted\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}	
	case 7: {
		printk("[program2] : get SIGBUS signal\n");
		printk("[program2] : child process has bus error\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}	
	case 8: {
		printk("[program2] : get SIGFPE signal\n");
		printk("[program2] : child process has floating-point exception\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}	
	case 9: {
		printk("[program2] : get SIGKILL signal\n");
		printk("[program2] : child process has forced-process termination\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}
	case 11: {
		printk("[program2] : get SIGSEGV signal\n");
		printk("[program2] : child process has illegal memory reference\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}
	case 13: {
		printk("[program2] : get SIGPIPE signal\n");
		printk("[program2] : child process writes to pipe with no readers\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}
	case 14: {
		printk("[program2] : get SIGALRM signal\n");
		printk("[program2] : child process is alarmed\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}
	case 15: {
		printk("[program2] : get SIGTERM signal\n");
		printk("[program2] : child process has process termination\n");
		printk("[program2] : The return signal is %d\n",
		       ((wopt.wo_stat) & 0x7f));
		break;
	}

	default: {
		// if (wo.wo_stat>>8==SIGSTOP){
		if (wifstopped(wopt.wo_stat)) {
			printk("[program2] : get SIGSTOP signal\n");
			printk("[program2] : child process has stopped process execution\n");
			printk("[program2] : The return signal is %d\n",
			       SIGSTOP);
		} else {
			printk("[program2] : The return signal is %d\n",
			       ((wopt.wo_stat) & 0x7f));
		}
	}
}
	put_pid(wo_pid_ptr);

	return;
}

static int __init program2_init(void)
{
	printk("[program2] : module_init\n");
	printk("[program2] : module_init create kthread start\n");

	//create a kthread
	struct task_struct *task = kthread_run(&my_fork, NULL, "MyThread");
	printk("[program2] : module_init kthread start\n");
	/* write your code here */

	/* create a kernel thread to run my_fork */

	return 0;
}

static void __exit program2_exit(void)
{
	printk("[program2] : module_exit\n");
}

module_init(program2_init);
module_exit(program2_exit);