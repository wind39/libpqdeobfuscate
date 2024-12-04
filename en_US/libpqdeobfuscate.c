#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <internal/postgres_fe.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <internal/libpq-int.h>


static int deobfuscate_pass(char *buf, int size, PGconn *conn);
typedef void (*PQsetSSLKeyPassHook_t) (PQsslKeyPassHook_OpenSSL_type hook);
typedef PQconninfoOption *(*PQconninfo_t)(PGconn *conn);
PQconninfo_t PQconninfo_func;


/*
 *  Function that runs through the arguments of the original connection
 *    string and returns the "sslpassword" value as a a "char *".
 *    This string needs to be de-allocated.
 *    If "sslpassword" does not exist in the connection string,
 *    return NULL.
 */
static char * get_sslpassword(PGconn * conn)
{
	PQconninfoOption *conninfo = (*PQconninfo_func)(conn);
	char * result = NULL;
	PQconninfoOption *cursor;
	for (cursor = conninfo; cursor && cursor->keyword; cursor++)
	{
		if (strcmp(cursor->keyword, "sslpassword") == 0)
		{
			if (cursor->val != NULL)
				result = strdup(cursor->val);
			break;
		}
	}
	PQconninfoFree(conninfo);
	return result;
}


/*
 *  Callback function that's executed every time a new connection is made.
 */
static int deobfuscate_pass(char *buf, int size, PGconn *conn)
{
	char * obfuscated;
	// We are hard-coding the real certificate key password
	char deobfuscated[] = "oe4keeP3";

	// Get the "sslpassword" value from the connection string
	obfuscated = get_sslpassword(conn);

	if (obfuscated != NULL)
	{
		// Real de-obfuscation would happen here
		//

		// De-allocate the string used to store the obfuscated password
		free(obfuscated);

		strncpy(buf, deobfuscated, strlen(deobfuscated) + 1);
		return strlen(buf);
	}
	else
	{
		buf[0] = '\0';
		return 0;
	}
}


/*
 *  Constructor function which is called when the library
 *    is loaded. It registers the callback function
 *    (in our case it's called "deobfuscate_pass") associated
 *    with the hook PQsetSSLKeyPassHook_OpenSSL.
 */
void sslpasslib_init() __attribute__ ((constructor));
void sslpasslib_init()
{
	PQsetSSLKeyPassHook_t PQsetSSLKeyPassHook;
	void * lpq_handle;

	lpq_handle = dlopen(NULL, RTLD_NOW|RTLD_GLOBAL);
	if (lpq_handle)
	{
		PQsetSSLKeyPassHook = dlsym(lpq_handle,
			"PQsetSSLKeyPassHook_OpenSSL");
		PQconninfo_func = dlsym(lpq_handle, "PQconninfo");
		if (PQsetSSLKeyPassHook)
			(*PQsetSSLKeyPassHook)(deobfuscate_pass);
	}
}
