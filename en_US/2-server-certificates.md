# 2- Server certificates

## 2.1- Root certificate

Let's connect to our certificate server container:

```
docker exec -it certserver bash
```

Let's create a key and a certificate signing request (CSR) for the root CA:

```
openssl req -new -nodes -text -out root.csr \
  -keyout root.key -subj "/CN=certserver"
```

We sign the request using the root key to create the root certificate, which is self-signed and valid for 10 years:

```
openssl x509 -req -in root.csr -text -days 3650 \
  -extfile /etc/ssl/openssl.cnf -extensions v3_ca \
  -signkey root.key -out root.crt
```


## 2.2- Server certificate

Still connected to our certificate server, we create a key and a CSR for the Postgres server:

```
openssl req -new -nodes -text -out server.csr \
  -keyout server.key -subj "/CN=pgserver"
```

We sign the request with the root key to create our server certificate, valid for 1 year:

```
openssl x509 -req -in server.csr -text -days 365 \
  -CA root.crt -CAkey root.key -CAcreateserial \
  -out server.crt
```


## 2.3- Copy the certificates to the server

Copy the root certificate, the server certificate and the server key from inside the certificate server container to your host:

```
docker cp certserver:/root/root.crt .
docker cp certserver:/root/server.crt .
docker cp certserver:/root/server.key .
```

Copy the root certificate, the server certificate and the server key from your host to inside the Postgres server container, into the PGDATA:

```
docker cp root.crt pgserver:/var/lib/pgsql/17/data/
docker cp server.crt pgserver:/var/lib/pgsql/17/data/
docker cp server.key pgserver:/var/lib/pgsql/17/data/
```

Connect into the client container:

```
docker exec -it client bash
```

Go into the user home directory:

```
cd ~
```

Create the folder `~/.postgresql/`:

```
mkdir ~/.postgresql/
chmod 700 ~/.postgresql/
```

From the host, copy the root certificate into the folder `~/.postgresql/`:

```
docker cp root.crt client:/root/.postgresql/
```


## 2.4- Postgres

We connect into the Postgres server container:

```
docker exec -it pgserver bash
```

The root certificate, the server certificate and the server key need to belong to the `postgres` user:

```
chown postgres:postgres /var/lib/pgsql/17/data/root.crt
chown postgres:postgres /var/lib/pgsql/17/data/server.crt
chown postgres:postgres /var/lib/pgsql/17/data/server.key
```

We become `postgres` user:

```
su - postgres
```

We enable SSL in Postgres:

```
cat >> /var/lib/pgsql/17/data/postgresql.conf << EOF
ssl = on
ssl_key_file = 'server.key'
ssl_cert_file = 'server.crt'
ssl_ca_file = 'root.crt'
EOF
```

Now we delete the following line from file `/var/lib/pgsql/17/data/pg_hba.conf`:

```
host myappdb myappuser 172.18.0.22/32 scram-sha-256
```

And add the following line into file `/var/lib/pgsql/17/data/pg_hba.conf`:

```
hostssl myappdb myappuser 172.18.0.22/32 scram-sha-256
```

Then we restart Postgres:

```
/usr/pgsql-17/bin/pg_ctl -D /var/lib/pgsql/17/data/ -l logfile restart
```


## 2.5- Client

Connect into the client container:

```
docker exec -it client bash
```

Connect into Postgres:

```
psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full"
```

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full"
psql (17.0)
SSL connection (protocol: TLSv1.3, cipher: TLS_AES_256_GCM_SHA384, compression: off, ALPN: postgresql)
Type "help" for help.

myappdb=> 
```
