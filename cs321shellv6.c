#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<unistd.h>
#include<wait.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#define MAXARG 10
#define MAXALEN 50
int cnt = 0, pind = -1, bgpCnt = 0, hisCnt = 0;
char* hst;
char** tokenize(char *cmdinp){
	int sz = 1;
	for(int i = 0; i<strlen(cmdinp)-1; i++){
		if(cmdinp[i] != ' ' && cmdinp[i+1] == ' ')
			sz++;
	}
	cnt = sz;
	char** argList = (char**)malloc(sz*sizeof(char*));
	for(int i = 0; i<sz; i++)
		argList[i] = (char*)malloc(MAXALEN*sizeof(char));
	int j = 0;
	bool b = false;
	for(int i = 0; i<sz; i++){
		bool spcs = true;
		while(cmdinp[j] == ' ')
			j++;
		int st = j;
		while(cmdinp[j] != ' ' && cmdinp[j] != '|'){
			if(j == strlen(cmdinp) - 1){
				b = true;
				break;
			}
			j++;
		}
		if(cmdinp[j] == '|'){
				pind = i;
				j++;
				spcs = false;
				argList[i][0] = '|';
				argList[i][1] = '\0';
		}
		if(spcs){
			strncpy(argList[i], &cmdinp[st], j - st);		
			argList[i][j-st] = '\0';
		}
		if(b)
			break;
	}
	return argList;
}
int isRedirection(char** argList, int n){
	for(int i = 1; i<n-1; i++){
		if(argList[i][0] == '<' || argList[i][0] == '>'){
			return i;
		}
	}
	return -1;
}
int cntSemiCol(char** argList, int n){
	int count = 0;	
	for(int i = 1; i<n-1; i++){
		if(argList[i][0] == ';'){
			count++;
		}
	}
	return count;
}
void bckgrdHndl(int s)
{
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0);
    bgpCnt--;
}
void execute(char** argList, int n){
	int status;
	bool bckgrd = false;
	if(argList[cnt-1][0] == '&'){
		bckgrd = true;
		cnt--;
		bgpCnt++;
		argList[cnt] = NULL;
		signal(SIGCHLD, bckgrdHndl);
	}
	pid_t pid = fork();
	if(pid == 0){
		int tmp = isRedirection(argList, n);
		bool chk = true;
		if(tmp != -1){
			if(argList[tmp][0] == '<'){
				dup2(open(argList[tmp+1], O_RDONLY | O_CREAT), 0);
				if(n > tmp + 2)
				dup2(open(argList[tmp+3], O_WRONLY | O_CREAT), 1);
			}			
			else
				dup2(open(argList[tmp+1], O_WRONLY | O_CREAT), 1);
			argList[tmp] = NULL;
		}		
		else if(pind != -1){
			int m = pind + 1;
			char** argListNew = (char**)malloc((n - pind)*sizeof(char*));
			for(int k = 0; k<n - pind; k++){
				argListNew[k] = (char*)malloc(MAXALEN*sizeof(char));
				if(k < n - pind - 1)			
					strcpy(argListNew[k], argList[m++]);
				else
					argListNew[k] = NULL;
			}
			int fd[2];
			pipe(fd);
			pid_t npid = fork();
			if (npid != 0){
	      			dup2(fd[1], 1);
	      			close(fd[0]);
				argList[pind] = NULL;
				execvp(argList[0], argList);   				
			}
	   		else{
				waitpid(npid, &status, 0);
	      			dup2(fd[0], 0);
	      			close(fd[1]);
				execvp(argListNew[0], argListNew);
				for(int k = 0; k<n - pind; k++)
					free(argListNew[k]);
				free(argListNew);
				chk = false;
			}
		}
		if(chk)
			execvp(argList[0], argList);			
	}
	else{
		if(!bckgrd)
			waitpid(pid, &status, 0);
		for(int i = 0; i<n; i++)
			free(argList[i]);
		free(argList);
	}
}
int getHisCnt(){
	close(open(hst, O_CREAT));
	FILE* fl = fopen(hst, "r");
	char tp[512];
	int count = 0;
	while(fgets(tp, 512, fl) != NULL)
		count++;
	fclose(fl);
	return count;
}
bool readHist(int num, char* cmdinp){
	if(num > 10 || num == 0 || num < -10)
		return false;
	int count = hisCnt;
	char tp[512];
	if(num < 0 && -1*count > num)
		return false;
	else if(num > 0 && count < num)
		return false;
	else{
		FILE* fl = fopen(hst, "r");
		int i = 1;		
		if(num < 0)
			i = count*-1;
		for(; i<num; i++){
			fgets(tp, 512, fl);
		}
		fgets(cmdinp, 512, fl);
		fclose(fl);
		return true;
	}
}
int typeCnvtr(const char* t){
		
	if(strcmp(t,"-d") == 0)
		return 0040000;
	else if(strcmp(t, "-f") == 0)
		return 0100000;
	else if(strcmp(t, "-c") == 0)
		return 0020000;
	else if(strcmp(t, "-b") == 0)
		return 0060000;
	else if(strcmp(t, "-p") == 0)
		return 0010000;
	else if(strcmp(t, "-S") == 0)
		return 0140000;
	else if(strcmp(t, "-h") == 0)
		return 0120000;
	else
		exit(1);
}
char* hndlIF(char** argList){
	if(strcmp(argList[0],"if") == 0 && strcmp(argList[1],"[") == 0 && strcmp(argList[4],"]") == 0){
		struct stat buf;
   		if (lstat(argList[3], &buf)<0){
      			perror("Error in stat");
      			exit(1);
   		}
		char tp[3][512];	
		for(int i = 0; i<3; i++){
			printf(">");
			fgets(tp[i], 512, stdin);
			if(i == 0 && (tp[0][0] != 't' || tp[0][1] != 'h' || tp[0][2] != 'e' || tp[0][3] != 'n'))
				return NULL;
			else if(i == 1 && (tp[1][0] != 'e' || tp[1][1] != 'l' || tp[1][2] != 's' || tp[1][3] != 'e'))
				return NULL;
			else if(i == 2 && (tp[2][0] != 'f' || tp[2][1] != 'i'))
				return NULL;
		}
		int ln = 1;
		if((buf.st_mode &  0170000) == typeCnvtr(argList[2]))
			ln = 0;
		char* cdip = (char*)malloc(MAXARG*MAXALEN*sizeof(char));
		strcpy(cdip, &tp[ln][5]);
		return cdip;
	}
	return NULL;
}
void wrtCmd(char* cmdinp){
	if(hisCnt < 11)
		hisCnt++;
	FILE* fl = fopen(hst, "r");
	char** cmd = (char**)malloc((hisCnt)*sizeof(char*));
	for(int k = 0; k<hisCnt; k++){
		cmd[k] = (char*)malloc(512*sizeof(char));
		if(hisCnt == 11 || k<hisCnt-1){
			fgets(cmd[k], 512, fl);
		}
		else
			strcpy(cmd[k], cmdinp);
	}
	if(hisCnt == 11){
		for(int k = 0; k<hisCnt - 1; k++)
			strcpy(cmd[k], cmd[k+1]);
		strcpy(cmd[hisCnt - 1], cmdinp);
	}
	fclose(fl);
	fl = fopen(hst, "w");
	for(int k = 0; k<hisCnt; k++)
		fputs(cmd[k], fl);
	fclose(fl);
}
int main(){
	signal(SIGINT,SIG_IGN);
	char c;
	char* cmdinp;
	hst = getcwd(NULL, 100);
	strcat(hst,"/.history.txt");
	char** argList;
	sigset_t new, old;
	while(true){
		hisCnt = getHisCnt();
		char* curDir = getcwd(NULL, 100);
		sigemptyset(&new);
		sigaddset(&new, SIGINT);
		sigprocmask(SIG_SETMASK,&new,&old);
		printf("cs321shell@%s:~%s:", getlogin(), curDir);
		cmdinp = (char*)malloc(MAXARG*MAXALEN*sizeof(char));
		c = EOF;
		if((c=getchar())==EOF){
			printf("\n");
			exit(0);
		}
		else{
			if(c == '\n')
				continue;
			cmdinp[0] = c;
		}
		fgets(&cmdinp[1], 512, stdin);
		if(c == '!'){
			int num = 1, in = 1;
			if(cmdinp[1] == '-')
				in++;		
			if(cmdinp[in] >= '1' && cmdinp[in] <= '9'){
				num = cmdinp[in] - 48;
				if(cmdinp[in] == '1' && cmdinp[in+1] == '0')
					num+=9;
				if(cmdinp[in+1] != '0' && strlen(cmdinp) > in+2){
					printf("Invalid Command!\n");
					continue;
				}
				if(in == 2)
					num*=-1;
				if(!readHist(num, cmdinp)){
					printf("Invalid Command!\n");
					continue;
				}
			}
			else{
				printf("Invalid Command!\n");
				continue;
			}
		}
		argList = tokenize(cmdinp);
		if(strcmp(argList[0], "if") == 0){
			cmdinp = hndlIF(argList);
			if(cmdinp == NULL){
				printf("Invalid Command!\n");					
				continue;
			}
			argList = tokenize(cmdinp);
			argList[2] = NULL;
		}
		else if(strcmp(argList[0],"exit") == 0){			
			wrtCmd(cmdinp);		
			exit(0);
		}
		else if(strcmp(argList[0],"cd") == 0){			
			if(argList[1][0] != '/')
				strcat(curDir, "/");
			strcat(curDir, argList[1]);			
			if(chdir(curDir) == -1)
				printf("Directory does not exist!");
			wrtCmd(cmdinp);
			continue;
		}
		else if(strcmp(argList[0],"jobs") == 0){
			int stt;
			pid_t pid = fork();
			if(pid == 0)
				execlp("ps","jobs","-ef",NULL);
			waitpid(pid, &stt, 0);
			wrtCmd(cmdinp);
			continue;		
		}
		else if(strcmp(argList[0],"kill") == 0){
			kill(atoi(argList[1]),SIGKILL);
			printf("Killed\n");
			wrtCmd(cmdinp);			
			continue;
		}
		else if(strcmp(argList[0],"help") == 0){
			printf("====== HELP ======\n");			
			printf("cd      cd [DIR]\n");
			printf("exit    exit\n");
			printf("jobs    jobs\n");
			printf("kill    kill [PID]\n");
			printf("help    help\n");
			wrtCmd(cmdinp);
			continue;
		}
		wrtCmd(cmdinp);			
		int j = 0, m = 0, st;
		int smCnt = cntSemiCol(argList, cnt);
		sigprocmask(SIG_SETMASK, &old, NULL);
		if(smCnt == 0)
			execute(argList, cnt);	
		else
			for(int i = 0; i <= smCnt; i++){
				st = j;
				if(i == smCnt)
					j = cnt;
				else
					while(argList[j][0] != ';')
						j++;
				char** argListNew = (char**)malloc((j - st)*sizeof(char*));
				for(int k = 0; k<j - st; k++){
					argListNew[k] = (char*)malloc(MAXALEN*sizeof(char));
					strcpy(argListNew[k], argList[m++]);
				}
				execute(argListNew, j - st);
				j++;
				m++;
			}
	}
}
