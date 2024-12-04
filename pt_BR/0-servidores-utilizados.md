# 0- Servidores utilizados


## 0.1- Containers

Primeiro vamos criar a rede Docker e um container para representar cada servidor:

```
docker network create --subnet=172.18.0.0/16 authtest

docker run -d \
  --name certserver \
  --net authtest \
  --ip 172.18.0.20 \
  --hostname certserver \
  --add-host pgserver:172.18.0.21 \
  --add-host client:172.18.0.22 \
  -it rockylinux:9 bash

docker run -d \
  --name pgserver \
  --net authtest \
  --ip 172.18.0.21 \
  --hostname pgserver \
  --add-host certserver:172.18.0.20 \
  --add-host client:172.18.0.22 \
  -it rockylinux:9 bash

docker run -d \
  --name client \
  --net authtest \
  --ip 172.18.0.22 \
  --hostname client \
  --add-host certserver:172.18.0.20 \
  --add-host pgserver:172.18.0.21 \
  -it rockylinux:9 bash
```


## 0.2- Servidor de certificados

Por ora, no servidor de certificados, nós só conectamos no container:

```
docker exec -it certserver bash
```

E, dentro do container, atualizamos os pacotes:

```
dnf -y update
```


## 0.3- Postgres

Agora conectamos no servidor Postgres:

```
docker exec -it pgserver bash
```

Dentro do container, atualizamos os pacotes e instalamos o PostgreSQL 17:

```
dnf -y update

dnf -y install https://download.postgresql.org/pub/repos/yum/reporpms/EL-9-x86_64/pgdg-redhat-repo-latest.noarch.rpm
dnf -qy module disable postgresql
dnf -y install postgresql17-server
```

Agora nos tornamos usuário `postgres`, criamos o PGDATA, configuramos o Postgres para receber requisições remotas e iniciamos o serviço:

```
su - postgres

/usr/pgsql-17/bin/initdb -D /var/lib/pgsql/17/data/

cat >> /var/lib/pgsql/17/data/postgresql.conf << EOF
listen_addresses = '*'
EOF

/usr/pgsql-17/bin/pg_ctl -D /var/lib/pgsql/17/data/ -l logfile start
```

Finalmente criamos o banco de dados e o usuário que serão utilizados pela nossa aplicação:

```
cat << EOF | psql
CREATE USER myappuser WITH PASSWORD 'ohsei7Ae';
CREATE DATABASE myappdb OWNER myappuser;
EOF
```


## 0.4- Cliente

Conectamos no container:

```
docker exec -it client bash
```

Atualizamos os pacotes e instalamos o Postgres client:

```
dnf -y update

dnf install -y https://download.postgresql.org/pub/repos/yum/reporpms/EL-9-x86_64/pgdg-redhat-repo-latest.noarch.rpm
dnf -qy module disable postgresql
dnf install -y postgresql17
```
