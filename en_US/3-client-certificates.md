# 3- Client certificates

## 3.1- Client certificate

Let's connect to our certificate server container:

```
docker exec -it certserver bash
```

We create a key and a CSR to our user:

```
openssl req -new -nodes -text -out myappuser.csr \
  -keyout myappuser.key -subj "/CN=myappuser"
```

We sign the request with the root key to create the client certificate, valid for 1 year:

```
openssl x509 -req -in myappuser.csr -text -days 365 \
  -CA root.crt -CAkey root.key -CAcreateserial \
  -out myappuser.crt
```


## 3.2- Copy the certificate to the client container

Copy the key and the client certificate to your host:

```
docker cp certserver:/root/myappuser.crt .
docker cp certserver:/root/myappuser.key .
```

Copy the key and the client certificate into the folder `~/.postgresql/` in the client container:

```
docker cp myappuser.crt client:/root/.postgresql/
docker cp myappuser.key client:/root/.postgresql/
```


## 3.3- pg_hba

We connect into the Postgres server container:

```
docker exec -it pgserver bash
```

We remove the following line from file `/var/lib/pgsql/17/data/pg_hba.conf`:

```
hostssl myappdb myappuser 172.18.0.22/32 scram-sha-256
```

And we add the following line into file `/var/lib/pgsql/17/data/pg_hba.conf`:

```
hostssl myappdb myappuser 172.18.0.22/32 scram-sha-256 clientcert=verify-full
```

Then we reload the configuration:

```
psql -c "SELECT pg_reload_conf()"
```


## 3.4- Client

We connect into the client container:

```
docker exec -it client bash
```

We connect to Postgres:

```
psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt"
```

Example:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt"
psql (17.0)
SSL connection (protocol: TLSv1.3, cipher: TLS_AES_256_GCM_SHA384, compression: off, ALPN: postgresql)
Type "help" for help.

myappdb=>
```
