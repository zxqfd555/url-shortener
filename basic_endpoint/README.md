Решил использовать в качестве фреймворка Crow: https://github.com/ipkn/crow

Как завести на Ubuntu:

 1. Поставить зависимости.

 `sudo apt-get install build-essential libtcmalloc-minimal4 && sudo ln -s /usr/lib/libtcmalloc_minimal.so.4 /usr/lib/libtcmalloc_minimal.so`

 2. Поставить boost.

 `sudo apt-get install libboost-all-dev`

 3. Взять большой crow.h отсюда: https://github.com/ipkn/crow/releases/download/v0.1/crow_all.h

 4. `make release` В целом там все довольно понятно со сборкой, надо слинковаться с чем-то из буста и с pthread.

 5. `./hello`

Походить в свежесозданный endpoint можно через `make test`, но вообще route там /hello.

