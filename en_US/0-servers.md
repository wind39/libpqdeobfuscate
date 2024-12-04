# 0- Servers


## 0.1- Containers

First let's create the Docker network and one container to represent each server:

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


## 0.2- Certificate Server

For now, on the certificate server, we only connect to the container:

```
docker exec -it certserver bash
```

And, inside the container, we update the packages:

```
dnf -y update
```


## 0.3- Postgres

Now we connect into the Postgres server container:

```
docker exec -it pgserver bash
```

Inside the container, we update the packages and install PostgreSQL 17:

```
dnf -y update

dnf -y install https://download.postgresql.org/pub/repos/yum/reporpms/EL-9-x86_64/pgdg-redhat-repo-latest.noarch.rpm
dnf -qy module disable postgresql
dnf -y install postgresql17-server
```

Now we become `postgres` user, create the PGDATA directory, configure Postgres to receive remote requests and start the service:

```
su - postgres

/usr/pgsql-17/bin/initdb -D /var/lib/pgsql/17/data/

cat >> /var/lib/pgsql/17/data/postgresql.conf << EOF
listen_addresses = '*'
EOF

/usr/pgsql-17/bin/pg_ctl -D /var/lib/pgsql/17/data/ -l logfile start
```

Finally, we create the database and user that will be used by our application:

```
cat << EOF | psql
CREATE USER myappuser WITH PASSWORD 'ohsei7Ae';
CREATE DATABASE myappdb OWNER myappuser;
EOF
```


## 0.4- Client

We connect into the client container:

```
docker exec -it client bash
```

We update the packages and install the Postgres client packages:

```
dnf -y update

dnf install -y https://download.postgresql.org/pub/repos/yum/reporpms/EL-9-x86_64/pgdg-redhat-repo-latest.noarch.rpm
dnf -qy module disable postgresql
dnf install -y postgresql17
```
