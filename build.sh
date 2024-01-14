#!/bin/sh

set -xe

clang -O3 -march=native -Wall -Wextra -o producer producer.c -lrdkafka -I./deps 
clang -O3 -march=native -Wall -Wextra -o test test.c -lrdkafka -I./deps 
clang -O3 -march=native -Wall -Wextra -o consumer consumer.c -lrdkafka