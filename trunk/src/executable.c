#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
/**
 * Attempts to return the error message associated with an exec error
 */
char * get_exec_error_msg(int RC, char *filename)
{
	switch ( RC ) 
	{
        case EACCES :
        {
            return "EACCES Permission denied";
        }
        case EPERM :
        {
            return "EPERM Not super-user";
        }
        case E2BIG :
        {
            return "E2BIG Arg list too long";
        }
        case ENOEXEC :
        {
            return "ENOEXEC Exec format error";
        }
        case EFAULT :
        {
            return "EFAULT Bad address";
        }
        case ENAMETOOLONG :
        {
            return "ENAMETOOLONG path name is too long     ";
        }
        case ENOENT :
        {
            return "ENOENT No such file or directory";
        }
        case ENOMEM :
        {
            return "ENOMEM Not enough core";
        }
        case ENOTDIR :
        {
            return "ENOTDIR Not a directory";
        }
        case ELOOP :
        {
            return "ELOOP Too many symbolic links";
        }
        case ETXTBSY :
        {
            return "ETXTBSY Text file busy";
        }
        case EIO :
        {
            return "EIO I/O error";
        }
        case ENFILE :
        {
            return "ENFILE Too many open files in system";
        }
        case EINVAL :
        {
            return "EINVAL Invalid argument";
        }
        case EISDIR :
        {
            return "EISDIR Is a directory";
        }
        case ELIBBAD :
        {
            return "ELIBBAD Accessing a corrupted shared lib";
        }
        default :
        {
            if (RC != 0) 
			{
				return strerror(RC);
        	}
		}
     }
	return NULL;
}
