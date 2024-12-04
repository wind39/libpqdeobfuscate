# 3- Client certificates

## 3.1- Certificado do cliente

Vamos conectar ao nosso servidor de certificados:

```
docker exec -it certserver bash
```

Criamos uma chave uma CSR para o usuário:

```
openssl req -new -nodes -text -out myappuser.csr \
  -keyout myappuser.key -subj "/CN=myappuser"
```

Assinamos a request com a chave root para criar o client certificate, válido por 1 ano:

```
openssl x509 -req -in myappuser.csr -text -days 365 \
  -CA root.crt -CAkey root.key -CAcreateserial \
  -out myappuser.crt
```


## 3.2- Copiar o certificado para o cliente

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


## 3.3- pg_hba

Conectamos no servidor Postgres:

```
docker exec -it pgserver bash
```

Removemos a seguinte linha do arquivo `/var/lib/pgsql/17/data/pg_hba.conf`:

```
hostssl myappdb myappuser 172.18.0.22/32 scram-sha-256
```

E adicionamos a seguinte linha ao arquivo `/var/lib/pgsql/17/data/pg_hba.conf`:

```
hostssl myappdb myappuser 172.18.0.22/32 scram-sha-256 clientcert=verify-full
```

Em seguida recarregamos a configuração:

```
psql -c "SELECT pg_reload_conf()"
```


## 3.4- Cliente

Conectamos no container cliente:

```
docker exec -it client bash
```

Conectamos no Postgres:

```
psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt"
```

Exemplo:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser sslmode=verify-full sslkey=/root/.postgresql/myappuser.key sslcert=/root/.postgresql/myappuser.crt"
psql (17.0)
SSL connection (protocol: TLSv1.3, cipher: TLS_AES_256_GCM_SHA384, compression: off, ALPN: postgresql)
Type "help" for help.

myappdb=>
```
