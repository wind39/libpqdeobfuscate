# 5- sslpassword (de)obfuscation

## 5.1- Instalar dependências

Vamos conectar no nosso servidor de certificados:

```
docker exec -it certserver bash
```

E rodar os seguintes comandos para instalar as dependências:

```
dnf -y update

dnf -y install https://download.postgresql.org/pub/repos/yum/reporpms/EL-9-x86_64/pgdg-redhat-repo-latest.noarch.rpm
dnf -qy module disable postgresql

dnf -y --enablerepo=crb install perl-IPC-Run

dnf -y install \
  libpq5-devel \
  openssl-devel \
  krb5-devel \
  postgresql17-devel
```


## 5.2- Copiar o código-fonte em C para o servidor de certificados

Na máquina host:

```
docker cp libpqdeobfuscate.c certserver:/root/
```


## 5.3- Compilar a biblioteca

No servidor de certificados, vamos compilar o código-fonte `.c` em um arquivo `.so`, isto é, uma biblioteca compartilhada:

```
cd ~

gcc -DUSE_OPENSSL \
  -I/usr/pgsql-17/include/ \
  -I/usr/pgsql-17/include/server/ \
  -L/usr/pgsql-17/include/lib/ \
  -L/usr/lib64/ -lpq \
  libpqdeobfuscate.c \
  -shared -fPIC \
  -o libpqdeobfuscate.so
```


## 5.4- Copiar a biblioteca para o container cliente

Copie a biblioteca que acabamos de compilar do servidor de certificados para o seu host:

```
docker cp certserver:/root/libpqdeobfuscate.so .
```

Agora copie a biblioteca do seu host para o container cliente:

```
docker cp libpqdeobfuscate.so client:/root/
```


## 5.5- Cliente

Primeiro tentamos conectar obfuscando a sslpassword, mas sem carregar a biblioteca de desofuscação:

```
psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt sslpassword=XXXXXX"
```

Não irá funcionar, exemplo:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt sslpassword=XXXXXX"
psql: error: connection to server at "pgserver" (172.18.0.21), port 5432 failed: could not load private key file "/root/.postgresql/myappuser.key": bad decrypt
```

Agora vamos carregar a `libpq` e a nossa nova biblioteca:

```
export LD_PRELOAD=/usr/pgsql-17/lib/libpq.so.5:/root/libpqdeobfuscate.so
```

Em seguida tentamos conectar novamente, dessa vez a biblioteca vai permitir conectar usando a `sslpassword` real, mesmo que na connection string ela esteja ofuscada:

```
[root@client ~]# export LD_PRELOAD=/usr/pgsql-17/lib/libpq.so.5:/root/libpqdeobfuscate.so
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt sslpassword=XXXXXX"
psql (17.0)
SSL connection (protocol: TLSv1.3, cipher: TLS_AES_256_GCM_SHA384, compression: off, ALPN: postgresql)
Type "help" for help.

myappdb=>
```


## 5.6- pg_service

Também funciona ofuscar a `sslpassword` no arquivo `~/.pg_service.conf`, para uma segurança ainda maior:

```
[root@client ~]# cat ~/.pg_service.conf
[myapp]
host=pgserver
port=5432
dbname=myappdb
user=myappuser
sslmode=verify-full
sslrootcert=/root/.postgresql/root.crt
sslcert=/root/.postgresql/myappuser.crt
sslkey=/root/.postgresql/myappuser.key
sslpassword=XXXXXX
```

Exemplo:

```
[root@client ~]# psql "service=myapp"
psql (17.0)
SSL connection (protocol: TLSv1.3, cipher: TLS_AES_256_GCM_SHA384, compression: off, ALPN: postgresql)
Type "help" for help.

myappdb=>
```
