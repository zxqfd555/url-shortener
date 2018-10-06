# Простейший endpoint на C++

В качестве фреймвока использован Crow: https://github.com/ipkn/crow

## Как завести на Ubuntu

 1. Поставить зависимости.

 `sudo apt-get install build-essential libtcmalloc-minimal4 && sudo ln -s /usr/lib/libtcmalloc_minimal.so.4 /usr/lib/libtcmalloc_minimal.so`

 2. Поставить boost.

 `sudo apt-get install libboost-all-dev`

 3. Взять большой crow.h отсюда: https://github.com/ipkn/crow/releases/download/v0.1/crow_all.h

 4. `make release` В целом там все довольно понятно со сборкой, надо слинковаться с чем-то из буста и с pthread.

 5. `./hello`

Походить в свежесозданный endpoint можно через `make test`, но вообще route там /hello.

## Как запустить с nginx

 1. Поставить nginx.
 
 `sudo apt-get install nginx-full`
 
 2. Сделать бэкап конфига.
 
 `sudo cp /etc/nginx/nginx.conf /etc/nginx/nginx-backup.conf`
 
 3. Подменить конфиг на свой.
 
 `sudo cp nginx.conf /etc/nginx/nginx.conf`
 
 4. Запустить.
 
 `sudo /etc/init.d/nginx start`
 
 5. Лог доступа будет писаться в `/usr/share/nginx/access.log`
 
 6. `curl "http://localhost/hw"` чтобы проверить, что все работает.
