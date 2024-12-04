# 1- Password-based

## 1.1- Postgres

We connect to the Postgres server container:

```
docker exec -it pgserver bash
```

We install our preferred editor:

```
dnf -y install vim
```

We become `postgres` user:

```
su - postgres
```

And we add the following line to the file `/var/lib/pgsql/17/data/pg_hba.conf`:

```
host myappdb myappuser 172.18.0.22/32 scram-sha-256
```

Then we reload Postgres:

```
psql -c "SELECT pg_reload_conf()"
```


## 1.2- Client

We connect to the client container:

```
docker exec -it client bash
```

If we try to connect without passing a password:

```
psql "host=pgserver port=5432 dbname=myappdb user=myappuser"
```

Postgres will ask to type a password, for example:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser"
Password for user myappuser:
psql (17.0)
Type "help" for help.

myappdb=>
```

This doesn't happen if we include the password in the connection string:

```
psql "host=pgserver port=5432 dbname=myappdb user=myappuser password='ohsei7Ae'"
```

For example:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser password='ohsei7Ae'"
psql (17.0)
Type "help" for help.

myappdb=>
```

It's possible to hide the password using the environment variable `PGPASSWORD`, so we won't need to include the password in the connection string:

```
export PGPASSWORD=ohsei7Ae

psql "host=pgserver port=5432 dbname=myappdb user=myappuser"
```

But the usual way is to use the file `~/.pgpass`:

```
cat > ~/.pgpass << EOF
pgserver:5432:myappdb:myappuser:ohsei7Ae
EOF
chmod 600 ~/.pgpass

psql "host=pgserver port=5432 dbname=myappdb user=myappuser"
```

The password doesn't need to be included in the connection string because it comes from file `~/.pgpass`:

```
[root@client ~]# psql "host=pgserver port=5432 dbname=myappdb user=myappuser"
psql (17.0)
Type "help" for help.

myappdb=>
```
