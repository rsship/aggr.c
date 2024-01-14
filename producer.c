#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "librdkafka/rdkafka.h"

const int TOTAL_MSG = 1000000;

static void
dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque)
{
  if (rkmessage->err)
    fprintf(stderr, "%% Message delivery failed: %s\n",
            rd_kafka_err2str(rkmessage->err));
  else
    fprintf(stderr,
            "%% Message delivered (%zd bytes, "
            "partition %" PRId32 ")\n",
            rkmessage->len, rkmessage->partition);
}

typedef struct
{
  const char *broker;
  const char *topic;
} Kafka;

int main(void)
{
  rd_kafka_t *rk;
  rd_kafka_conf_t *conf;
  char errstr[512];
  char msg[512];

  Kafka kafka_config = {
      .broker = "localhost:9092",
      .topic = "my_topic",
  };

  conf = rd_kafka_conf_new();

  if (rd_kafka_conf_set(conf, "bootstrap.servers", kafka_config.broker, errstr,
                        sizeof(errstr)) != RD_KAFKA_CONF_OK)
  {
    fprintf(stderr, "%s\n", errstr);
    return 1;
  }

  rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);

  rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
  if (!rk)
  {
    fprintf(stderr, "could not  to create new producer: %s\n",
            errstr);
    return 1;
  }

  for(int i = 0;i < TOTAL_MSG;++i)
  {
    sprintf(msg, "message -> %d", i);
    size_t len = strlen(msg);
    rd_kafka_resp_err_t err;

  retry:
    err = rd_kafka_producev(
        rk,
        RD_KAFKA_V_TOPIC(kafka_config.topic),
        RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
        RD_KAFKA_V_VALUE(msg, len),
        RD_KAFKA_V_OPAQUE(NULL),
        RD_KAFKA_V_END);

    if (err)
    {
      fprintf(stderr,
              "%% could not produce to topic %s: %s\n", kafka_config.topic,
              rd_kafka_err2str(err));

      if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL)
      {
        rd_kafka_poll(rk,
                      1000);
        goto retry;
      }
    }
    else
    {
      fprintf(stdout,
              "%% Enqueued message (%zd bytes) "
              "for topic %s\n",
              len, kafka_config.topic);
    }

    rd_kafka_poll(rk, 0);
  }

  fprintf(stderr, "%% Flushing final messages..\n");
  rd_kafka_flush(rk, 10 * 1000);

  if (rd_kafka_outq_len(rk) > 0)
    fprintf(stderr, "%% %d message(s) were not delivered\n",
            rd_kafka_outq_len(rk));

  rd_kafka_destroy(rk);

  return 0;
}