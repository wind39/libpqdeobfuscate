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
 * Função que percorre os argumentos da connection string original
 *   e retorna o valor da "sslpassword" como um "char *".
 *   Esta string precisa ser desalocada.
 *   Se "sslpassword" não existir na connection string,
 *   retorna NULL.
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
 * Função callback que é executada toda vez que uma conexão é feita.
 */
static int deobfuscate_pass(char *buf, int size, PGconn *conn)
{
	char * obfuscated;
	// Estamos fixando a senha real do certificado
	char deobfuscated[] = "oe4keeP3";

	// Obter o valor da "sslpassword" da connection string
	obfuscated = get_sslpassword(conn);

	if (obfuscated != NULL)
	{
		// De-obfuscação real aconteceria aqui
		//

		// Desalocar a string usada para armazenar a senha obfuscada
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
 *  Função construtura que é chamada quando a biblioteca
 *    é carregada. Registra a função callback
 *    (no nosso caso é chamada "deobfuscate_pass")
 *    associada ao hook PQsetSSLKeyPassHook_OpenSSL.
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
