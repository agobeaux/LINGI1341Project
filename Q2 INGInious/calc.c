#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/*
    -a ADD
        Ajoute ADD à la valeur courante
    -s SUB
        Soustrait SUB à la valeur courante
    -m MUL
        Multiplie la valeur courante par MUL
    -d DIV
        Divise la valeur courante par DIV
    -I
        Incrémente la valeur courante de 1
    -D
        Décrémente la valeur courante de 1
*/

/*
 * calc -IIIII affiche 5
 * calc -a 3 -Id2 affiche 2
 * calc -a 0xa affiche 10
 * calc -m3 -d 2 -DDDD 5 affiche -4.00000
 */

int main(int argc, char* argv[]){
	char c;
	double total = 0.0;
	char* lastArg = NULL;
	double value;
	while ((c = getopt(argc, argv, "a:s:m:d:ID")) != -1){
		switch(c){
			case 'a' :
				value = strtod(optarg,NULL);
				if(!value && optarg[0] != '0'){
					fprintf(stderr, "Not a number\n");
					return -1;
				}
				total += value;
				//printf("opt arg : %s, value : %f\n", optarg, strtod(optarg,NULL));
				lastArg = optarg;
				break;
			case 's' :
				value = strtod(optarg,NULL);
				if(!value && optarg[0] != '0'){
					fprintf(stderr, "Not a number\n");
					return -1;
				}
				total -= value;
				//printf("opt arg : %s, value : %f\n", optarg, strtod(optarg,NULL));
				lastArg = optarg;
				break;
			case 'm' :
				value = strtod(optarg,NULL);
				if(!value && optarg[0] != '0'){
					fprintf(stderr, "Not a number\n");
					return -1;
				}
				total *= value;
				//printf("opt arg : %s, value : %f\n", optarg, strtod(optarg,NULL));
				lastArg = optarg;
				break;
			case 'd' :
				value = strtod(optarg,NULL);
				if(!value && optarg[0] != '0'){
					fprintf(stderr, "Not a number\n");
					return -1;
				}
                if(!value){
                    fprintf(stderr, "Only Chuck Norris can divide by 0\n");
                    return -1;
                }
				total /= value;
				lastArg = optarg;
				break;
			case 'I' :
				total++;
				lastArg = "-";
				break;
			case 'D' :
				total--;
				lastArg = "-";
				break;
			case '?' :
				fprintf(stderr, "Wrong option\n");
				return -1;
		}
	}
    int precision = 0;
	if(argc > 1 && argv[argc-1][0] != '-' && strcmp(argv[argc-1],lastArg)){
		precision = atoi(argv[argc-1]);
	}
	printf("%.*f\n",precision,total);
		return 0;
}
