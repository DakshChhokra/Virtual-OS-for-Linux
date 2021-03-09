#include "p_errno.h"

/*
 * Function that prints an error message based on the value of p_errno
 * @param message, a user inputted message
 */
void p_perror(char *message) {
	switch(p_errno) {
		case 0:
			printf("%s: SUCCESS\n", message);
			break;

	   	case -1:
	   		printf("%s: no such process\n", message);
	      	break;
		
	   	case -2:
	      	printf("%s: invalid argument\n", message);
	      	break;

	    case -3:
	      	printf("%s: cannot allocate more memory\n", message);
	      	break;
	}
}