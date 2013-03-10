/*Jonathan Drouillard
 * File Age Report
 * age.c
 * Modified: 11/1/11
 */

/*
 *
 * ==4562==     in use at exit: 0 bytes in 0 blocks
 * ==4562==   total heap usage: 85 allocs, 85 frees, 38,757 bytes allocated
 * ==4562==
 * ==4562== All heap blocks were freed -- no leaks are possible
 *
 */

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

/*Structure that holds our getopt settings*/
struct globalArgs_t {
	size_t hours;
	size_t minutes;
	size_t seconds;
	size_t reverse;
} globalArgs;

/*String of chars we are looking for in getopt*/
static const char *optString = "hmr";

/*Function which does the output processing for files*/
void lenTime(char** a, size_t len, size_t type, char *dirName);

/*Comparables for qsort*/
int asc(void const *a, void const *b);
int desc(void const *a, void const *b);

/*Global constants for our flags*/
#define  HM 0
#define  H  1
#define  M  2
#define  S  3

int main(int argc, char **argv){
	/*Assume default of seconds*/
	globalArgs.hours = 0;
	globalArgs.minutes = 0;
	globalArgs.reverse = 0;
	globalArgs.seconds = 1;

	int opt = 0, i = 0;

	opt = getopt(argc, argv, optString);
	while(opt != -1) {
		switch(opt) {
			case 'h': /*turn off seconds, turn on hours*/
				globalArgs.seconds = 0;
				globalArgs.hours = 1;
				break;
			case 'm': /*turn off seconds, turn on minutes*/
				globalArgs.seconds = 0;
				globalArgs.minutes = 1;
				break;
			case 'r': /*if r, only have to set it*/
				globalArgs.reverse = 1;
				break;
			case '?':
				printf("Unknown flag, please use h, m, or r\n");
				exit(1);
				break;
		} /*repeat until out of opts*/
		opt = getopt(argc, argv, optString);
	}

	DIR *dp;
	size_t buffsz = 1024, strPos = 0;
	char *buff, *dirName, **strArray;

	struct dirent *de;

	/*Allocate memory for the current working directory and grab it*/
	if((buff = (char *)malloc(buffsz)) != NULL) {
		dirName = getcwd(buff, buffsz);
	}
	else {
		printf("Error allocating memory.\n");
		exit(1);
	}

	/*Attempt to open the cwd*/
	if((dp = opendir(dirName)) == NULL) {
		printf("Error opening %s.", dirName);
		exit(1);
	}
	
	/*Build our dynamic string array*/
	size_t strCap = 5;
	strArray = malloc(strCap*sizeof(*strArray));
	while((de = readdir(dp)) != NULL) {
		if(strPos == strCap) {
			/*Double capacity if full*/
			strCap = strCap * 2;
			strArray = realloc(strArray, strCap*sizeof(*strArray));
			if(strArray == NULL) {
				printf("Out of memory\n");
				exit(1);
			}
		}
		/*Avoid . and .. files*/
		if((strcmp(de->d_name,".") != 0)
               && (strcmp(de->d_name,"..") != 0)) {
			/*Add cwd + filename together*/
			/*-1, only need one '\0'*/
			char *path = (char*)malloc(strlen(dirName)
				   + sizeof(de->d_name) -1);
			strcpy(path, dirName);
			strcat(path, "/"); /*Add the / to the end of the dir*/
			strcat(path, de->d_name); /*Append the filename*/
			/*Allocate memory*/
			strArray[strPos] = malloc((strlen(path) + 1)*sizeof(char));
			strcpy(strArray[strPos], path);
			strPos++; /*Creates off by one*/
			if(path != NULL) {
				free(path);
				path = NULL;
			}
		}
	}
	/*Sort the arrays before sending them for printing*/
	if(globalArgs.reverse == 1) { /*sort in reverse order*/
		qsort(strArray, strPos, sizeof(char*),asc);
	}
	if(globalArgs.reverse == 0) { /*normal sort/default*/
		qsort(strArray, strPos, sizeof(char*),desc);
	}

	/*check for -hm*/
	if(globalArgs.hours == 1 && globalArgs.minutes == 1) {
		lenTime(strArray, strPos, HM, dirName);
	}else if(globalArgs.hours == 1) { /*-h*/
                lenTime(strArray, strPos, H, dirName);
	}else if(globalArgs.minutes == 1) { /*-m*/
                lenTime(strArray, strPos, M, dirName);
	}else { /*default: seconds*/
		lenTime(strArray, strPos, S, dirName);
	}

	/*Clean up each row of the array*/
	for(i = 0; i < strPos; i++) {
		free(strArray[i]);
	}
	/*Clean up more allocated memory*/
	free(strArray);
	free(buff);

	/*Cleans up the streams*/
        if(closedir(dp) == -1) {
                printf("Error closing %s.", dirName);
                exit(1);
        }


	return 0;

}

/*Comparables for qsort*/

int desc(void const *a, void const *b) {
	const char **ia = (const char **)a;
	const char **ib = (const char **)b;
	return -strcmp(*ia, *ib);
}

int asc(void const *a, void const *b) {
	const char **ia = (const char **)a;
        const char **ib = (const char **)b;
	return strcmp(*ia, *ib);
}

void lenTime(char** strArray, size_t strPos, size_t type, char *dirName) {
	int maxLen = 0;
	int minLen = 0;
	const size_t BUFFSZ = 1024;
	int i = 0;
	/*Find the longest string*/
        for(i = 0; i < strPos; i++) {
                struct stat statbuf; /*to pull mod time*/
                stat(strArray[i],&statbuf);
                time_t ltime; /*to subtract from mod time*/
                time( &ltime ); /*prep work*/
                ctime( &ltime ); /*formatting*/
		char strhr[BUFFSZ], strmin[BUFFSZ];
		size_t ihours, isecs;
		double mins, dhours;
		switch(type) {
			case HM:
				/*Calculate hours and fractional minutes*/
				ihours = (int)(difftime(ltime, statbuf.st_mtime)/3600);
				mins = (((int)(difftime(ltime, statbuf.st_mtime))%3600)/60.0);
				/*Store the double/unsigned int in a char str*/
				snprintf(strhr, BUFFSZ, "%d", ihours);
				snprintf(strmin, BUFFSZ, "%1.1f", mins);
				/*Calculate max length of mins and ihours for use in formatting*/
				if(strlen(strmin) > maxLen) {
					maxLen = (strlen(strmin));
				}
				if(strlen(strhr) > minLen) {
					minLen = (strlen(strhr));
				}
				break;
			case H:/*-h*/
				/*Calc fractional hours*/
				dhours = (difftime(ltime, statbuf.st_mtime)/3600);
				/*Store in a char str*/
				snprintf(strhr, BUFFSZ, "%1.1f", dhours);
				/*Check if this is the longest length age*/
				if(strlen(strhr) > maxLen) {
                                        maxLen = (strlen(strhr));
                                }
				break;
			case M:/*-m*/
				/*Calc mins*/
				mins = (difftime(ltime, statbuf.st_mtime)/60);
                                /*Store in a char str*/
				snprintf(strmin, BUFFSZ, "%1.1f", mins);
                                /*Check if this is the longest length age*/
				if(strlen(strmin) > maxLen) {
                                        maxLen = (strlen(strmin));
                                }
				break;
			default: /*seconds*/
				isecs = (int)difftime(ltime, statbuf.st_mtime);
                                snprintf(strmin, BUFFSZ, "%d", isecs);
                                if(strlen(strmin) > maxLen) {
                                        maxLen = (strlen(strmin));
                                }
                                break;
		}
	}
	switch(type) {
		case HM:
			for(i = 0; i < strPos; i++) { /*Traverse the string array*/
			        struct stat statbuf; /*to pull mod time*/
		                stat(strArray[i],&statbuf);
	        	        time_t ltime; /*to subtract from mod time*/
	                	time( &ltime ); /*prep work*/
		                ctime( &ltime ); /*formatting*/

				/*Recalc hours and minutes*/
				int hours = (int)((difftime(ltime, statbuf.st_mtime)/60)/60);
				double minutes = (((int)(difftime(ltime, statbuf.st_mtime))%3600)/60.0);
				/*Place in a string*/
				char strhr[BUFFSZ];
				snprintf(strhr, BUFFSZ, "%d", hours);
				/*Format it using the rules from the requirements document*/
        		        printf("%d %-*.1f%s\n", hours, (int)(maxLen+(minLen-strlen(strhr))),  minutes, strArray[i]+(strlen(dirName)+1) );
			}
			break;
		case H:
                        for(i = 0; i < strPos; i++) { /*Traverse the string array*/
                                struct stat statbuf; /*to pull mod time*/
                                stat(strArray[i],&statbuf);
                                time_t ltime; /*to subtract from mod time*/
                                time( &ltime ); /*prep work*/
                                ctime( &ltime ); /*formatting*/

                                double hours = (difftime(ltime, statbuf.st_mtime)/3600);
                                printf("%-*.1f %s\n", maxLen, hours, strArray[i]+(strlen(dirName)+1) );
                        }
			break;
		case M:
			for(i = 0; i < strPos; i++) {
                                struct stat statbuf; /*to pull mod time*/
                                stat(strArray[i],&statbuf);
                                time_t ltime; /*to subtract from mod time*/
                                time( &ltime ); /*prep work*/
                                ctime( &ltime ); /*formatting*/

                                double minutes = (difftime(ltime, statbuf.st_mtime)/60);
                                printf("%-*.1f %s\n", maxLen, minutes, strArray[i]+(strlen(dirName)+1) );
                        }
			break;
		default: /*seconds*/
			for(i = 0; i < strPos; i++) {
                                struct stat statbuf; /*to pull mod time*/
                                stat(strArray[i],&statbuf);
                                time_t ltime; /*to subtract from mod time*/
                                time( &ltime ); /*prep work*/
                                ctime( &ltime ); /*formatting*/

                                int seconds = (int)difftime(ltime, statbuf.st_mtime);
                                printf("%-*d %s\n", maxLen, seconds, strArray[i]+(strlen(dirName)+1) );
                        }
			break;
	}
}
