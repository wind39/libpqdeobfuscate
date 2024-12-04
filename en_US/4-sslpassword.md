# 4- sslpassword

## 4.1- Client certificate, but with encrypted key

Let's connect to our certificate server container:

```
docker exec -it certserver bash
```

Let's create an encrypted key for the user:

```
openssl genrsa -passout pass:oe4keeP3 -aes256 -out myappuser.key 2048
```

Check if the key is really encrypted:

```
[root@certserver ~]# cat myappuser.key
-----BEGIN ENCRYPTED PRIVATE KEY-----
...
-----END ENCRYPTED PRIVATE KEY-----
```

Test the key password:

```
openssl pkey -pubout -in myappuser.key -passin pass:oe4keeP3
```

If the password is correct:

```
[root@certserver ~]# openssl pkey -pubout -in myappuser.key -passin pass:oe4keeP3
-----BEGIN PUBLIC KEY-----
...
-----END PUBLIC KEY-----
```

If the password is incorrect:

```
[root@certserver ~]# openssl pkey -pubout -in myappuser.key -passin pass:aaa
Could not read key from myappuser.key
806BD5DAE37F0000:error:1608010C:STORE routines:ossl_store_handle_load_result:unsupported:crypto/store/store_result.c:151:
806BD5DAE37F0000:error:1C800064:Provider routines:ossl_cipher_unpadblock:bad decrypt:providers/implementations/ciphers/ciphercommon_block.c:124:
806BD5DAE37F0000:error:11800074:PKCS12 routines:PKCS12_pbe_crypt_ex:pkcs12 cipherfinal error:crypto/pkcs12/p12_decr.c:86:maybe wrong password
```

Create a certificate signing request (CSR) for the user:

```
openssl req -passin pass:oe4keeP3 -new -sha256 -key myappuser.key -out myappuser.csr -subj "/CN=myappuser"
```

Sign the request with the root CA to create a client certificate for the user, valid for 1 year:

```
openssl x509 -req -in myappuser.csr -CA root.crt -CAkey root.key -CAcreateserial -out myappuser.crt -days 365 -sha256
```

Check that the client certificate and the client key are equivalent:

```
openssl pkey -pubout -in myappuser.key -passin pass:oe4keeP3 | openssl sha256
openssl x509 -pubkey -in myappuser.crt -noout | openssl sha256
```

Should return the same value, for example:

```
[root@certserver ~]# openssl pkey -pubout -in myappuser.key -passin pass:oe4keeP3 | openssl sha256
SHA256(stdin)= 507c731f69587fef43bdbd15ec151501233dac09bb2eeff919f14256db1bc7b1
[root@certserver ~]# openssl x509 -pubkey -in myappuser.crt -noout | openssl sha256
SHA256(stdin)= 507c731f69587fef43bdbd15ec151501233dac09bb2eeff919f14256db1bc7b1
```


## 4.2- Copy the certificate to the client container

Copy the client key and certificate to your host:

```
docker cp certserver:/root/myappuser.crt .
docker cp certserver:/root/myappuser.key .
```

Copy the client key and certificate to folder `~/.postgresql/` inside the client container:

```
docker cp myappuser.crt client:/root/.postgresql/
docker cp myappuser.key client:/root/.postgresql/
```


## 4.3- pg_hba

We connect to the Postgres server:

```
docker exec -it pgserver bash
```

We remove the following line from file `/var/lib/pgsql/17/data/pg_hba.conf`:

```
hostssl myappdb myappuser 172.18.0.22/32 scram-sha-256 clientcert=verify-full
```

And we add the following line to file `/var/lib/pgsql/17/data/pg_hba.conf`:

```
hostssl myappdb myappuser 172.18.0.22/32 cert
```

Which is equivalent to:

```
hostssl myappdb myappuser 172.18.0.22/32 trust clientcert=verify-full
```

Then we reload the configuration:

```
psql -c "SELECT pg_reload_conf()"
```


## 4.4- Client

We connect into the client container:

```
docker exec -it client bash
```

We remove file `~/.pgpass`:

```
rm ~/.pgpass
```

We connect to Postgres:

```
psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt sslpassword='oe4keeP3'"
```

Example:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt sslpassword='oe4keeP3'"
psql (17.0)
SSL connection (protocol: TLSv1.3, cipher: TLS_AES_256_GCM_SHA384, compression: off, ALPN: postgresql)
Type "help" for help.

myappdb=>
```


## 4.5- pg_service

We connect into the client container:

```
docker exec -it client bash
```

We create file `~/.pg_service.conf`:

```
cat > ~/.pg_service.conf << EOF
[myapp]
host=pgserver
port=5432
dbname=myappdb
user=myappuser
password=ohsei7Ae
sslmode=verify-full
sslrootcert=/root/.postgresql/root.crt
sslcert=/root/.postgresql/myappuser.crt
sslkey=/root/.postgresql/myappuser.key
sslpassword=oe4keeP3
EOF

chmod 600 ~/.pg_service.conf
```

We connect to Postgres using only the service name:

```
psql "service=myapp"
```

For example:

```
[root@client ~]# psql "service=myapp"
psql (17.0)
SSL connection (protocol: TLSv1.3, cipher: TLS_AES_256_GCM_SHA384, compression: off, ALPN: postgresql)
Type "help" for help.

myappdb=>
```
