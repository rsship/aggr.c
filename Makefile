build: 
	@clang -O3 -Wall -Wextra -o producer producer.c -I./deps -I/usr/local/include/ -lrdkafka
	@clang -O3 -Wall -Wextra -o consumer consumer.c -I./deps -I/usr/local/include/ -lrdkafka

clear: 
	@rm -rf producer
	@rm -rf consumer
