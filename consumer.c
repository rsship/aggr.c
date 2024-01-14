#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <librdkafka/rdkafka.h>

static volatile sig_atomic_t run = 1;
const int TOPICS_COUNT = 1;
static void stop() { run = 0; }

typedef struct {
  const char *brokers;
  const char *group_id;
  const char *topics[TOPICS_COUNT];
} Kafka;

static int is_printable(const char *buf, size_t size) {
  size_t i;
  for (i = 0; i < size; i++)
    if (!isprint((int)buf[i]))
      return 0;
  return 1;
}

void consume(rd_kafka_t *rk) {
  while (run) {
    rd_kafka_message_t *rkm = rd_kafka_consumer_poll(rk, 100);
    if (!rkm)
      continue;
    if (rkm->err) {
      fprintf(stderr, "%% Consumer error: %s\n", rd_kafka_message_errstr(rkm));
      rd_kafka_message_destroy(rkm);
      continue;
    }

    printf("Message on %s [%" PRId32 "] at offset %" PRId64
           " (leader epoch %" PRId32 "):\n",
           rd_kafka_topic_name(rkm->rkt), rkm->partition, rkm->offset,
           rd_kafka_message_leader_epoch(rkm));

    if (rkm->key && is_printable(rkm->key, rkm->key_len))
      printf(" Key: %.*s\n", (int)rkm->key_len, (const char *)rkm->key);
    else if (rkm->key)
      printf(" Key: (%d bytes)\n", (int)rkm->key_len);

    if (rkm->payload && is_printable(rkm->payload, rkm->len))
      printf(" Value: %.*s\n", (int)rkm->len, (const char *)rkm->payload);
    else if (rkm->payload)
      printf(" Value: (%d bytes)\n", (int)rkm->len);

    rd_kafka_message_destroy(rkm);
  }
}

rd_kafka_conf_t *set_configs(Kafka *conf, char *err_str) {
  rd_kafka_conf_t *kafka_config = rd_kafka_conf_new();
  if (rd_kafka_conf_set(kafka_config, "bootstrap.servers", conf->brokers,
                        err_str, sizeof(err_str)) != RD_KAFKA_CONF_OK) {
    fprintf(stderr, "could not set brokers\n");
    rd_kafka_conf_destroy(kafka_config);
    return NULL;
  }

  if (rd_kafka_conf_set(kafka_config, "group.id", conf->group_id, err_str,
                        sizeof(err_str)) != RD_KAFKA_CONF_OK) {
    fprintf(stderr, "could not set group id\n");
    rd_kafka_conf_destroy(kafka_config);
    return NULL;
  }

  if (rd_kafka_conf_set(kafka_config, "auto.offset.reset", "earliest", err_str,
                        sizeof(err_str)) != RD_KAFKA_CONF_OK) {
    rd_kafka_conf_destroy(kafka_config);
    return NULL;
  }

  if (rd_kafka_conf_set(kafka_config, "enable.auto.commit", "true", err_str,
                        sizeof(err_str)) != RD_KAFKA_CONF_OK) {
    fprintf(stderr, "could not set enable.auto.commit %s\n", err_str);
    rd_kafka_conf_destroy(kafka_config);
    return NULL;
  }

  return kafka_config;
}

int main() {
  Kafka kafka_config = {
      .brokers = "localhost:9092",
      .group_id = "test_group",
      .topics = {"my_topic"},
  };
  char err_str[512];

  rd_kafka_conf_t *conf = set_configs(&kafka_config, err_str);
  if (conf == NULL) {
    printf("could not set configs \n");
    return 1;
  }
  rd_kafka_t *rk =
      rd_kafka_new(RD_KAFKA_CONSUMER, conf, err_str, sizeof(err_str));

  if (!rk) {
    fprintf(stderr, "could not create consumer %s\n", err_str);
    return 1;
  }

  conf = NULL;
  rd_kafka_poll_set_consumer(rk);

  rd_kafka_topic_partition_list_t *subscriptions =
      rd_kafka_topic_partition_list_new(TOPICS_COUNT);
  for (int i = 0; i < TOPICS_COUNT; ++i) {
    rd_kafka_topic_partition_list_add(subscriptions, kafka_config.topics[i],
                                      RD_KAFKA_PARTITION_UA);
  }

  rd_kafka_resp_err_t err = rd_kafka_subscribe(rk, subscriptions);
  if (err) {
    fprintf(stderr, "Could not subscribe to %d topics: %s\n",
            subscriptions->cnt, rd_kafka_err2str(err));
    rd_kafka_topic_partition_list_destroy(subscriptions);
    rd_kafka_destroy(rk);
    return 1;
  }

  fprintf(stdout, "Subs to %d topics, waiting for messages... \n",
          subscriptions->cnt);
  rd_kafka_topic_partition_list_destroy(subscriptions);

  signal(SIGINT, stop);

  consume(rk);

  fprintf(stdout, "Closing Consumer\n");
  rd_kafka_consumer_close(rk);
  rd_kafka_destroy(rk);
  return 0;
}
