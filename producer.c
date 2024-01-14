#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "librdkafka/rdkafka.h"
#define PARSER_IMPLEMENTATION
#include "parser.h"

const int TOTAL_MSG = 10000000;

static void
dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque)
{
  if (rkmessage->err)
    fprintf(stdout, "%% Message delivery failed: %s\n",
            rd_kafka_err2str(rkmessage->err));
  else
    fprintf(stderr,
            "%% Message delivered (%zd bytes, "
            "partition %" PRId32 ")\n",
            rkmessage->len, rkmessage->partition);
}

typedef struct
{
  char *broker;
  char *topic;
} Kafka;

typedef struct
{
  size_t group;
  String_View text;
} Data;

typedef struct
{
  Data *items;
  size_t count;
  size_t capacity;
} Datas;

Datas parse_csv_data(String_View content)
{
  size_t lines_count = 0;
  Datas datas = {0};

  for (; content.count > 0; ++lines_count)
  {
    String_View line = sv_chop_by_delim(&content, '\n');
    if (lines_count == 0)
      continue;
    String_View group = sv_chop_by_delim(&line, ',');
    size_t group_idx = *group.data - '0' - 1;

    da_append(&datas, ((Data){
                             .group = group_idx,
                             .text = line,
                         }));
  }
  return datas;
}

int main(void)
{
  char errstr[512];
  char msg[512];

  Kafka kafka_config = {
      .broker = "localhost:9092",
      .topic = "my_topic",
  };

  rd_kafka_conf_t *conf = rd_kafka_conf_new();

  if (rd_kafka_conf_set(conf, "bootstrap.servers", kafka_config.broker, errstr,
                        sizeof(errstr)) != RD_KAFKA_CONF_OK)
  {
    fprintf(stderr, "%s\n", errstr);
    return 1;
  }

  rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);

  rd_kafka_t *rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
  if (!rk)
  {
    fprintf(stderr, "could not  to create new producer: %s\n",
            errstr);
    return 1;
  }

  // NOTE: READ WHOLE DATA
  const char *csv_path = "./data/train.csv";
  String_Builder csv_content = {0};
  if(!read_entire_file(csv_path, &csv_content)) return 1;

  String_View sv = {
    .count = csv_content.count, 
    .data = csv_content.items,
  };
  Datas csv_datas = parse_csv_data(sv);

  for (int i = 0; i < TOTAL_MSG; ++i)
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

  fprintf(stdout, "%% Flushing final messages..\n");
  rd_kafka_flush(rk, 10 * 1000);

  if (rd_kafka_outq_len(rk) > 0)
    fprintf(stderr, "%% %d message(s) were not delivered\n",
            rd_kafka_outq_len(rk));

  rd_kafka_destroy(rk);

  return 0;
}