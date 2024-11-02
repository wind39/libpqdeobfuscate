# 1- Password-based

## 1.1- Postgres

Conectamos no servidor Postgres:

```
docker exec -it pgserver bash
```

Instalamos o nosso editor preferido:

```
dnf -y install vim
```

Nos tornamos usuário `postgres`:

```
su - postgres
```

E adicionamos a seguinte linha ao arquivo `/var/lib/pgsql/17/data/pg_hba.conf`:

```
host myappdb myappuser 172.18.0.22/32 scram-sha-256
```

Em seguida, fazemos um reload no Postgres:

```
psql -c "SELECT pg_reload_conf()"
```


## 1.2- Cliente

Conectamos no cliente:

```
docker exec -it client bash
```

Se tentarmos conectar sem informar a senha:

```
psql "host=pgserver port=5432 dbname=myappdb user=myappuser"
```

O Postgres vai pedir para digitarmos a senha, por exemplo:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser"
Password for user myappuser:
psql (17.0)
Type "help" for help.

myappdb=>
```

Isso não acontece se informarmos a senha na connection string:

```
psql "host=pgserver port=5432 dbname=myappdb user=myappuser password='ohsei7Ae'"
```

Por exemplo:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser password='ohsei7Ae'"
psql (17.0)
Type "help" for help.

myappdb=>
```

É possível ocultar a senha usando a variável de ambiente `PGPASSWORD`, e então não precisaremos passar a senha na connection string:

```
export PGPASSWORD=ohsei7Ae

psql "host=pgserver port=5432 dbname=myappdb user=myappuser"
```

Mas a forma mais comum é utilizar o arquivo `~/.pgpass`:

```
cat > ~/.pgpass << EOF
pgserver:5432:myappdb:myappuser:ohsei7Ae
EOF
chmod 600 ~/.pgpass

psql "host=pgserver port=5432 dbname=myappdb user=myappuser"
```

A senha não precisa ser informada na connection string porque ela vem do arquivo `~/.pgpass`:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser"
psql (17.0)
Type "help" for help.

myappdb=>
```
