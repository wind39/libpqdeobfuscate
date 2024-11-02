# 4- sslpassword

## 4.1- Certificado do cliente, porém com chave criptografada

Vamos conectar ao nosso servidor de certificados:

```
docker exec -it certserver bash
```

Vamos criar uma chave criptografada para o usuário:

```
openssl genrsa -passout pass:oe4keeP3 -aes256 -out myappuser.key 2048
```

Verifique que a chave é realmente criptograda:

```
[root@certserver ~]# cat myappuser.key
-----BEGIN ENCRYPTED PRIVATE KEY-----
...
-----END ENCRYPTED PRIVATE KEY-----
```

Teste a senha da chave:

```
openssl pkey -pubout -in myappuser.key -passin pass:oe4keeP3
```

Se a senha estiver correta:

```
[root@certserver ~]# openssl pkey -pubout -in myappuser.key -passin pass:oe4keeP3
-----BEGIN PUBLIC KEY-----
...
-----END PUBLIC KEY-----
```

Se a senha estiver incorreta:

```
[root@certserver ~]# openssl pkey -pubout -in myappuser.key -passin pass:aaa
Could not read key from myappuser.key
806BD5DAE37F0000:error:1608010C:STORE routines:ossl_store_handle_load_result:unsupported:crypto/store/store_result.c:151:
806BD5DAE37F0000:error:1C800064:Provider routines:ossl_cipher_unpadblock:bad decrypt:providers/implementations/ciphers/ciphercommon_block.c:124:
806BD5DAE37F0000:error:11800074:PKCS12 routines:PKCS12_pbe_crypt_ex:pkcs12 cipherfinal error:crypto/pkcs12/p12_decr.c:86:maybe wrong password
```

Crie uma certificate signing request (CSR) para o usuário:

```
openssl req -passin pass:oe4keeP3 -new -sha256 -key myappuser.key -out myappuser.csr -subj "/CN=myappuser"
```

Assine a request com o root CA pra criar um client certificate para o usuário, válido por 1 ano:

```
openssl x509 -req -in myappuser.csr -CA root.crt -CAkey root.key -CAcreateserial -out myappuser.crt -days 365 -sha256
```

Verifique que o client certificate e a client key são equivalentes:

```
openssl pkey -pubout -in myappuser.key -passin pass:oe4keeP3 | openssl sha256
openssl x509 -pubkey -in myappuser.crt -noout | openssl sha256
```

Devem retornar o mesmo valor, por exemplo:

```
[root@certserver ~]# openssl pkey -pubout -in myappuser.key -passin pass:oe4keeP3 | openssl sha256
SHA256(stdin)= 507c731f69587fef43bdbd15ec151501233dac09bb2eeff919f14256db1bc7b1
[root@certserver ~]# openssl x509 -pubkey -in myappuser.crt -noout | openssl sha256
SHA256(stdin)= 507c731f69587fef43bdbd15ec151501233dac09bb2eeff919f14256db1bc7b1
```


## 4.2- Copiar o certificado para o cliente

Copie a chave e o certificado do cliente para o seu host:

```
docker cp certserver:/root/myappuser.crt .
docker cp certserver:/root/myappuser.key .
```

Copie a chave e o certificado do cliente para a pasta `~/.postgresql/` do container cliente:

```
docker cp myappuser.crt client:/root/.postgresql/
docker cp myappuser.key client:/root/.postgresql/
```


## 4.3- pg_hba

Conectamos no servidor Postgres:

```
docker exec -it pgserver bash
```

Removemos a seguinte linha do arquivo `/var/lib/pgsql/17/data/pg_hba.conf`:

```
hostssl myappdb myappuser 172.18.0.22/32 scram-sha-256 clientcert=verify-full
```

E adicionamos a seguinte linha ao arquivo `/var/lib/pgsql/17/data/pg_hba.conf`:

```
hostssl myappdb myappuser 172.18.0.22/32 cert
```

Que é o equivalente a:

```
hostssl myappdb myappuser 172.18.0.22/32 trust clientcert=verify-full
```

Em seguida recarregamos a configuração:

```
psql -c "SELECT pg_reload_conf()"
```


## 4.4- Cliente

Conectamos no container cliente:

```
docker exec -it client bash
```

Removemos o arquivo `~/.pgpass`:

```
rm ~/.pgpass
```

Conectamos no Postgres:

```
psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt sslpassword='oe4keeP3'"
```

Exemplo:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt sslpassword='oe4keeP3'"
psql (17.0)
SSL connection (protocol: TLSv1.3, cipher: TLS_AES_256_GCM_SHA384, compression: off, ALPN: postgresql)
Type "help" for help.

myappdb=>
```


## 4.5- pg_service

Conectamos no container cliente:

```
docker exec -it client bash
```

Criamos o arquivo `~/.pg_service.conf`:

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

Conectamos no Postgres somente usando o nome do serviço:

```
psql "service=myapp"
```

Por exemplo:

```
[root@client ~]# psql "service=myapp"
psql (17.0)
SSL connection (protocol: TLSv1.3, cipher: TLS_AES_256_GCM_SHA384, compression: off, ALPN: postgresql)
Type "help" for help.

myappdb=>
```
