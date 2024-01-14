#!/bin/sh
set -xe
clang -O3 -march=native -Wall -Wextra -o producer producer.c -lrdkafka 
clang -O3 -march=native -Wall -Wextra -o consumer consumer.c -lrdkafka