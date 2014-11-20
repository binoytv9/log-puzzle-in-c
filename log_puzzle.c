#include<stdio.h>
#include<string.h>
#include<stdlib.h>

/* to create directory */
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>

/* to download images */
#include <curl/curl.h>
#include <curl/easy.h>

char *itoa(int num);
int returnTail(char *s)	;
int match(char *b,char *p);
int strCmp(char *a,char *b);
char *endsInPattern(char *s);
struct urlnode *talloc(void);
int contains(char *u,char *pat);
void printTree(struct urlnode *node);
void geturl(char *h,char *b,char *url);
char *reverse(char *s,int start,int stop);
void imageDownloader(char *url,char *outfilename);
void download_to_dir(struct urlnode *p,char *dir,int *i);
struct urlnode *url_insert(struct urlnode *p, char *url);
struct urlnode *puzzle_urls(char *file, struct urlnode *root);
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);

struct urlnode{
	char *url;
	struct urlnode *left;
	struct urlnode *right;
};

main(int argc, char *argv[])
{
	int j;
	int i=0;
	FILE *fp;
	int todir = 0;
	struct stat st = {0};
	char imgsrc[500] = {'\0'};
	char dir[500],logfile[500];
	struct urlnode *root = NULL;
	char indexname[100] = {'\0'};

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

		printf("\n\ncreating index.html ...\n");
		strcat(indexname,dir);
		strcat(indexname,"/index.html");
		fp = fopen(indexname,"w");
		fputs("<verbatim>\n<html>\n<body>\n",fp);
		for(j=1;j<i;++j){
			strcat(imgsrc,"<img src=\"img");
			strcat(imgsrc,itoa(j));
			strcat(imgsrc,"\">");
		}
		fputs(imgsrc,fp);
		fputs("\n</body>\n</html>\n",fp);
		fclose(fp);
	}
	else{
		printTree(root);
		printf("\n\n");
	}

}

/* returns a tree containing all the urls with word 'puzzle' in it*/
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

int match(char *b,char *p)
{
	if(strlen(b) < strlen(p))
		return 0;
	while(*b && *p)
		if(*b++ != *p++)
			return 0;
	return 1;
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

/* insert a new url in a tree */
struct urlnode *url_insert(struct urlnode *p, char *url)
{
	int cond;

	if( p == NULL){
		p = talloc();
		p->url = strdup(url);
		p->left = p->right = NULL;
	}
	else if((cond = strCmp(url,p->url)) < 0)
		p->left = url_insert(p->left,url);
	else if(cond > 0)
		p->right = url_insert(p->right,url);
/*	else
		 duplicate!!! do nothing, discard the url
*/

	return p;
}

struct urlnode *talloc(void)
{
	return (struct urlnode *)malloc(sizeof(struct urlnode));
}

int strCmp(char *a,char *b)
{
	int ia,ib;
	int ta,tb;

	ta = returnTail(a);
	tb = returnTail(b);

	a = endsInPattern(a+ta);
	b = endsInPattern(b+tb);

	return strcmp(a,b);
}

int returnTail(char *s)		/* returns the string after the last '/' */
{
	int i;
	int l = strlen(s);

	for(i=l-1;i>=0;--i)
		if(s[i] == '/')
			return i + 1;
}

/*	if the url ends in the pattern "-wordchars-wordchars.jpg", 
 *	e.g. "http://example.com/foo/puzzle/bar-abab-baaa.jpg", 
 *	then the url will be represented by the second word in the 
 *	sort (e.g. "baaa"). otherwise with the full url
 */
char *endsInPattern(char *s)
{
	char *a,*w;
	char word[100];

	a = s;
	w = word;
	while(*a && (*a++ != '-'));
	if(!*a)
		return s;
	while(isalpha(*a))
		a++;
	if(*a++ != '-')
		return s;
	while(isalpha(*a))
		*w++ = *a++;
	*w = '\0';
	return strdup(word);
}

/* download each url to a directory specified by the user */
void download_to_dir(struct urlnode *p,char *dir,int *i)
{
	char filename[500] = {'\0'};

	if(p != NULL){
		download_to_dir(p->left,dir,i);

		strcat(filename,dir);
		strcat(filename,"/img");
		strcat(filename,itoa(*i));
		printf("\ndownloading... %s\nsaving as... %s",p->url,filename);
		imageDownloader(p->url,filename);
		*i += 1;

		download_to_dir(p->right,dir,i);
	}
}

/* return string representation of an integer */
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

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

/* download each image and store with name outfilename */
void imageDownloader(char *url,char *outfilename)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outfilename,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);	/* to follow redirections in url */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
    }   
}

void printTree(struct urlnode *node)
{
	if(node != NULL){
		printTree(node->left);
		printf("\n%s",node->url);
		printTree(node->right);
	}
}
