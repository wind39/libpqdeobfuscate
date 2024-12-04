# 2- Server certificates

## 2.1- Certificado raiz

Vamos conectar ao nosso servidor de certificados:

```
docker exec -it certserver bash
```

Vamos criar uma chave e um certificate signing request (CSR) para a root CA:

```
openssl req -new -nodes -text -out root.csr \
  -keyout root.key -subj "/CN=certserver"
```

Assinamos a request com a chave root pra criar o certificado raiz, self-signed, válido por 10 anos:

```
openssl x509 -req -in root.csr -text -days 3650 \
  -extfile /etc/ssl/openssl.cnf -extensions v3_ca \
  -signkey root.key -out root.crt
```


## 2.2- Certificado do servidor

Ainda no nosso servidor de certificados, criamos uma chave uma CSR para o Postgres server:

```
openssl req -new -nodes -text -out server.csr \
  -keyout server.key -subj "/CN=pgserver"
```

Assinamos a request com a chave root para criar o server certificate, válido por 1 ano:

```
openssl x509 -req -in server.csr -text -days 365 \
  -CA root.crt -CAkey root.key -CAcreateserial \
  -out server.crt
```


## 2.3- Copiar os certificados para o servidor

Copie o certificado raiz, o certificado do servidor e a chave do servidor de dentro do container servidor de certificados para o seu host:

```
docker cp certserver:/root/root.crt .
docker cp certserver:/root/server.crt .
docker cp certserver:/root/server.key .
```

Copie o certificado raiz, o certificado do servidor e a chave do servidor do seu host para dentro do container servidor Postgres, dentro da PGDATA:

```
docker cp root.crt pgserver:/var/lib/pgsql/17/data/
docker cp server.crt pgserver:/var/lib/pgsql/17/data/
docker cp server.key pgserver:/var/lib/pgsql/17/data/
```

Conecte no container cliente:

```
docker exec -it client bash
```

Navegue para a pasta home do usuário:

```
cd ~
```

Crie a pasta `~/.postgresql/`:

```
mkdir ~/.postgresql/
chmod 700 ~/.postgresql/
```

A partir do host, copie o certificado raiz para dentro da pasta `~/.postgresql/`

```
docker cp root.crt client:/root/.postgresql/
```


## 2.4- Postgres

Conectamos no servidor Postgres:

```
docker exec -it pgserver bash
```

O certificado raiz, o certificado do servidor e a chave do servidor precisam pertencer ao usuário `postgres`:

```
chown postgres:postgres /var/lib/pgsql/17/data/root.crt
chown postgres:postgres /var/lib/pgsql/17/data/server.crt
chown postgres:postgres /var/lib/pgsql/17/data/server.key
```

Nos tornamos usuário `postgres`:

```
su - postgres
```

Habilitamos SSL no Postgres:

```
cat >> /var/lib/pgsql/17/data/postgresql.conf << EOF
ssl = on
ssl_key_file = 'server.key'
ssl_cert_file = 'server.crt'
ssl_ca_file = 'root.crt'
EOF
```

Removemos a seguinte linha do arquivo `/var/lib/pgsql/17/data/pg_hba.conf`:

```
host myappdb myappuser 172.18.0.22/32 scram-sha-256
```

E adicionamos a seguinte linha ao arquivo `/var/lib/pgsql/17/data/pg_hba.conf`:

```
hostssl myappdb myappuser 172.18.0.22/32 scram-sha-256
```

Em seguida, reiniciamos o Postgres:

```
/usr/pgsql-17/bin/pg_ctl -D /var/lib/pgsql/17/data/ -l logfile restart
```


## 2.5- Cliente

Conecte no container cliente:

```
docker exec -it client bash
```

Conecte no Postgres:

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
