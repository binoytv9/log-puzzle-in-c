#include<stdio.h>
#include<string.h>
#include<stdlib.h>

/* to create directory */
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>

char *itoa(int num);
int match(char *b,char *p);
struct urlnode *talloc(void);
int contains(char *u,char *pat);
void printTree(struct urlnode *node);
void geturl(char *h,char *b,char *url);
char *reverse(char *s,int start,int stop);
void download_to_dir(struct urlnode *p,char *dir,int *i);
struct urlnode *url_insert(struct urlnode *p, char *url);
struct urlnode *puzzle_urls(char *file, struct urlnode *root);

struct urlnode{
	char *url;
	struct urlnode *left;
	struct urlnode *right;
};

main(int argc, char *argv[])
{
	int i=0;
	int todir = 0;
	struct stat st = {0};
	char dir[500],logfile[500];
	struct urlnode *root = NULL;

	if(argc == 1){
		printf("\nusage : ./log_puzzle.out [--todir dir] logfilename\n\n");
		exit(1);
	}
	if(strcmp(*++argv,"--todir") == 0){
		todir = 1;
		if(--argc > 1)
			strcpy(dir,*++argv);
		else{
			printf("\ndir and logfilename needed");
			printf("\nusage : ./log_puzzle.out [--todir dir] logfilename\n\n");
			exit(1);
		}
		++argv;
		--argc;
	}

	if(--argc == 1)
		strcpy(logfile,*argv);
	else{
		printf("\nlogfilename needed");
		printf("\nusage : ./log_puzzle.out [--todir dir] logfilename\n\n");
		exit(1);
	}

	root = puzzle_urls(logfile,root);

	if(todir){
		if(stat(dir,&st) == -1)					/* if dir doesn't exists */
			mkdir(dir,S_IRWXU | S_IRWXG | S_IRWXO);		/* create it		 */
		download_to_dir(root,dir,&i);
	}
	else
		printTree(root);
	printf("\n\n");
}

void download_to_dir(struct urlnode *p,char *dir,int *i)
{
	char filename[500] = {'\0'};

	if(p != NULL){
		//FILE *fp;
		download_to_dir(p->left,dir,i);

		strcat(filename,dir);
		strcat(filename,"/img");
		strcat(filename,itoa(*i));
		printf("\n\n%s\n%s",p->url,filename);

		*i += 1;
		download_to_dir(p->right,dir,i);
		//fp = fopen(filename,"wb");
		//fclose(fp);
	}
}

char *itoa(int num)
{
	char array[100],*ar;

	ar = array;
	if(num == 0)
		*ar++ = '0';
	while(num){
		*ar++ = num%10 + '0';
		num /= 10;
	}
	*ar = '\0';
	return reverse(array,0,strlen(array)-1);
}

char *reverse(char *s,int start,int stop)
{
	char tmp;

	if(start >= stop)
		return strdup(s);

	tmp = s[start];
	s[start] = s[stop];
	s[stop] = tmp;

	return reverse(s,start+1,stop-1);
}

struct urlnode *puzzle_urls(char *file, struct urlnode *root)
{
	FILE *fp;
	int len = 0;
	char url[500];
	char buf[500],*b,*h;
	char hostname[100];

	b=file;
	h=hostname;
	strcpy(h,"http://www.");		/* adding "http://www." */
	h += strlen("http://www.");
	while(!match(b++,"_"));			/* extracting the hostname from filename */
	while(*h++ = *b++);			/* appending hostname */


	if((fp = fopen(file,"r")) == NULL){
		printf("\n\nInvalid logfilename : file doesn't exist\n\n");
		exit(1);
	}
	while(fgets(buf,500,fp) != NULL){
		geturl(hostname,buf,url);
		if(contains(url,"puzzle"))	/* if url contain the word 'puzzle' */
			root = url_insert(root,url);
	}
	fclose(fp);
	return root;
}

int contains(char *u,char *pat)
{
	char *p;
	if(strlen(u) < strlen(p))
		return 0;
	for( p = pat ; *u && *p ; ++u ){
		if(*u == *p)
			++p;
		else
			p=pat;
	}
	if(!*p)
		return 1;
	return 0;
}

struct urlnode *url_insert(struct urlnode *p, char *url)
{
	int cond;

	if( p == NULL){
		p = talloc();
		p->url = strdup(url);
		p->left = p->right = NULL;
	}
	else if((cond = strcmp(url,p->url)) < 0)
		p->left = url_insert(p->left,url);
	else if(cond > 0)
		p->right = url_insert(p->right,url);
/*	else
		 duplicate!!! do nothing, discard the url
*/

	return p;
}


void geturl(char *h,char *b,char *url)
{
	char *p;

	p=url;
	while(*b && !match(b++,"\"GET "));
	if(!*b){			/* if url doesn't contain '"GET ' */
		p='\0';
		return;
	}
	b += strlen("\"GET ") - 1;

	while(*h)			/* joining the hostname */
		*p++ = *h++;
	while(!isspace(*b))		/* url contains only non whitespace characters :p */
		*p++ = *b++;
	*p = '\0';
}

int match(char *b,char *p)
{
	if(strlen(b) < strlen(p))
		return 0;
	while(*b && *p)
		if(*b++ != *p++)
			return 0;
	return 1;
}

struct urlnode *talloc(void)
{
	return (struct urlnode *)malloc(sizeof(struct urlnode));
}

void printTree(struct urlnode *node)
{
	if(node != NULL){
		printTree(node->left);
		printf("\n%s",node->url);
		printTree(node->right);
	}
}
