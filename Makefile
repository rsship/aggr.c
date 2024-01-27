build: 
	@clang -O3 -march=native -Wall -Wextra -o producer producer.c -I./deps -lrdkafka
	@clang -O3 -march=native -Wall -Wextra -o consumer consumer.c -I./deps -lrdkafka

clear: 
	@rm -rf producer
	@rm -rf consumer
