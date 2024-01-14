producer: 
	@clang -O3 -march=native -Wall -Wextra -o producer producer.c -lrdkafka 
consumer: 
	@clang -O3 -march=native -Wall -Wextra -o consumer consumer.c -lrdkafka

clear: 
	@rm -rf producer
	@rm -rf consumer
