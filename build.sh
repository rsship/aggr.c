#!/bin/sh
set -xe

CC -O3 -march=native -Wall -Wextra -o producer producer.c  -I./deps -lrdkafka
CC -O3 -march=native -Wall -Wextra -o consumer consumer.c -I./deps -lrdkafka

