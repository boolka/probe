#ifndef PROBE_H
#define PROBE_H

#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

#define DEFAULT_SERVICE_PROTOCOL "tcp"
#define DEFAULT_RETRY_COUNT 5
#define DEFAULT_TIMEOUT 1
#define PROBE_VERSION "0.1.0"

typedef int socket_t;

typedef enum SERVICE_STATE
{
  AVAILABLE,
  UNAVAILABLE,
  UNKNOWN_PROTOCOL,
  UNKNOWN_HOST,
  UNKNOWN_SERVICE,
  INVALID_IP,
} SERVICE_STATE;

typedef struct probe_conf
{
  size_t retry_count;
  size_t timeout;
} probe_conf_t;

void
probe_config(size_t retry_count, size_t timeout);

SERVICE_STATE
ipv4_port_probe(char* ipv4, in_port_t port, char* protocol);

SERVICE_STATE
ipv4_service_probe(char* ipv4, char* service, char* protocol);

SERVICE_STATE
host_service_probe(char* host, char* service, char* protocol);

SERVICE_STATE
host_port_probe(char* host, in_port_t port, char* protocol);

char*
probe_version();

#endif
