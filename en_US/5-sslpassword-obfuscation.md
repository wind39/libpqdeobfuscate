# 5- sslpassword (de)obfuscation

## 5.1- Install dependencies

Let's connect into our certificate server container:

```
docker exec -it certserver bash
```

And run the following commands to install the dependencies:

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


## 5.2- Copy the source code in C to the certificate server

On the host machine:

```
docker cp libpqdeobfuscate.c certserver:/root/
```


## 5.3- Compile the library

On the certificate server, let's compile the source code `.c` file into a `.so` file, i.e., a shared library:

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


## 5.4- Copy the library to the client container

Copy the library we just compiled from the certificate servers to your host:

```
docker cp certserver:/root/libpqdeobfuscate.so .
```

Now copy the library from your host into the client container:

```
docker cp libpqdeobfuscate.so client:/root/
```


## 5.5- Client

In the client container, first we try to connect obfuscating the `sslpassword`, but without loading the deobfuscation library:

```
psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt sslpassword=XXXXXX"
```

It won't work:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt sslpassword=XXXXXX"
psql: error: connection to server at "pgserver" (172.18.0.21), port 5432 failed: could not load private key file "/root/.postgresql/myappuser.key": bad decrypt
```

Now let's load `libpq` and our new library:

```
export LD_PRELOAD=/usr/pgsql-17/lib/libpq.so.5:/root/libpqdeobfuscate.so
```

Then we try to connect again, this time the library will allow connection using the real `sslpassword`, even if in the connection string the `sslpassword` is obfuscated:

```
[root@client ~]# export LD_PRELOAD=/usr/pgsql-17/lib/libpq.so.5:/root/libpqdeobfuscate.so
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt sslpassword=XXXXXX"
psql (17.0)
SSL connection (protocol: TLSv1.3, cipher: TLS_AES_256_GCM_SHA384, compression: off, ALPN: postgresql)
Type "help" for help.

myappdb=>
```


## 5.6- pg_service

It also works to obfuscate the `sslpassword` in file `~/.pg_service.conf`, for greater security:

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

Example:

```
[root@client ~]# psql "service=myapp"
psql (17.0)
SSL connection (protocol: TLSv1.3, cipher: TLS_AES_256_GCM_SHA384, compression: off, ALPN: postgresql)
Type "help" for help.

myappdb=>
```
