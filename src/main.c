#include "probe.h"
#include <getopt.h>
#include <netdb.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_OPT_LEN_LIM 255

regex_t regex;
char* ip_re =
  "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|["
  "01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-"
  "5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$";

char host_or_ip[MAX_OPT_LEN_LIM], service[MAX_OPT_LEN_LIM];
in_port_t port;
size_t retry = DEFAULT_RETRY_COUNT, timeout = DEFAULT_TIMEOUT;
SERVICE_STATE service_state;

static char* help_msg =
  "Simplest possible solution to check service availability.\n\n"
  "The first thing probe is doing is trying to determine the host. When it "
  "is "
  "unavailable then it failures.\n"
  "After this probe will try to connect to host that many times that `retry` "
  "& "
  "`timeout` tells.\n"
  "On success probe will return `0` and `1` on failure.\n\n"
  "Don't use the `0.0.0.0` address.\n\n"
  "Usage: probe [OPTIONS] [HOST]\n\n"
  "\tHOST - host to connect to\n\n"
  "Options:\n"
  "\t-s, --service\t\t - service to connect to\n"
  "\t-p, --port\t\t - port to connect to\n"
  "\t-r, --retry\t\t - retry count\n"
  "\t-t, --timeout\t\t - timeout between retryings\n"
  "\t-h, --help\t\t - this help message\n"
  "\t-v, --version\t\t - current application version\n\n"
  "Examples:\n"
  "\tprobe --service=http localhost\n"
  "\tprobe --port=8080 localhost\n"
  "\tprobe --timeout=3 --retry=5 --service=https example.com\n";

static char* short_options = "s:p:r:t:hv";
static struct option long_options[] = {
  { "service", required_argument, NULL, 's' },
  { "port", required_argument, NULL, 'p' },
  { "retry", required_argument, NULL, 'r' },
  { "timeout", required_argument, NULL, 't' },
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { 0, 0, 0, 0 }
};

int
main(int argc, char** argv)
{
  int c, r;
  struct option* option;

  r = regcomp(&regex, ip_re, REG_EXTENDED);

  if (r != 0) {
    fprintf(stderr, "Regex compile fail: %d\n", r);
    exit(EXIT_FAILURE);
  }

  size_t host_ip_len = strlen(argv[argc - 1]);

  if (host_ip_len > MAX_OPT_LEN_LIM) {
    fprintf(stderr,
            "Host value is too long. Use only %u characters at max.\n",
            MAX_OPT_LEN_LIM);
    exit(EXIT_FAILURE);
  }

  while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) !=
         -1) {
    if (c == '?') {
      continue;
    }

    if (optarg != NULL) {
      size_t opt_len = strlen(optarg);

      if (opt_len > MAX_OPT_LEN_LIM) {
        fprintf(stderr,
                "Option value is too long. Use only %u characters at max.\n",
                MAX_OPT_LEN_LIM);
        exit(EXIT_FAILURE);
      }
    }

    switch (c) {
      case 'v': {
        puts(probe_version());
        exit(EXIT_SUCCESS);
      }
      case 'h': {
        printf(help_msg);
        exit(EXIT_SUCCESS);
      }
      case 's': {
        strncpy(service, optarg, MAX_OPT_LEN_LIM);
        break;
      }
      case 'p': {
        port = (in_port_t)atoi(optarg);
        break;
      }
      case 'r': {
        retry = (size_t)atoi(optarg);
        break;
      }
      case 't': {
        timeout = (size_t)atoi(optarg);
        break;
      }
    }
  }

  if (argc < 3) {
    fputs("Not enough arguments.\n", stderr);
    exit(EXIT_FAILURE);
  }

  strncpy(host_or_ip, argv[argc - 1], MAX_OPT_LEN_LIM);

#ifdef DEBUG
  printf("Host or IP argument: %s\n", host_or_ip);

  if (strlen(service) != 0) {
    printf("Service argument: %s\n", service);
  }

  if (port != 0) {
    printf("Port argument: %u\n", port);
  }

  if (timeout != 0) {
    printf("Timeout argument: %llu\n", timeout);
  }

  if (retry != 0) {
    printf("Retry argument: %llu\n", retry);
  }
#endif

  probe_config(retry, timeout);

  r = regexec(&regex, host_or_ip, 0, NULL, 0);

  // IP + service
  if (r == 0 && strlen(service) != 0) {
    service_state = ipv4_service_probe(host_or_ip, service, NULL);

    switch (service_state) {
      case AVAILABLE: {
        printf(
          "Service \"%s\" on ip \"%s\" is available.\n", service, host_or_ip);
        break;
      }
      case UNAVAILABLE: {
        fprintf(stderr,
                "Service \"%s\" on ip \"%s\" is unavailable.\n",
                service,
                host_or_ip);
        break;
      }
      case UNKNOWN_HOST: {
        fprintf(stderr, "Lookup for host \"%s\" is failed.\n", host_or_ip);
        break;
      }
      case INVALID_IP: {
        fprintf(stderr, "IP \"%s\" is invalid.\n", host_or_ip);
        break;
      }
      case UNKNOWN_SERVICE: {
        fprintf(stderr, "Service \"%s\" is not well known\n", service);
        break;
      }
      default: {
        fprintf(stderr, "Unknown error %d\n", service_state);
        break;
      }
    }
    // IP + port
  } else if (r == 0 && port != 0) {
    service_state = ipv4_port_probe(host_or_ip, port, NULL);

    switch (service_state) {
      case AVAILABLE: {
        printf("Port \"%d\" on ip \"%s\" is available.\n", port, host_or_ip);
        break;
      }
      case UNAVAILABLE: {
        fprintf(stderr,
                "Port \"%d\" on ip \"%s\" is unavailable.\n",
                port,
                host_or_ip);
        break;
      }
      case INVALID_IP: {
        fprintf(stderr, "IP \"%s\" is invalid.\n", host_or_ip);
        break;
      }
      case UNKNOWN_HOST: {
        fprintf(stderr, "Lookup for host \"%s\" is failed.\n", host_or_ip);
        break;
      }
      default: {
        fprintf(stderr, "Unknown error %d\n", service_state);
        break;
      }
    }
    // host + service
  } else if (strlen(host_or_ip) != 0 && strlen(service) != 0) {
    service_state = host_service_probe(host_or_ip, service, NULL);

    switch (service_state) {
      case AVAILABLE: {
        printf(
          "Service \"%s\" on host \"%s\" is available.\n", service, host_or_ip);
        break;
      }
      case UNAVAILABLE: {
        fprintf(stderr,
                "Service \"%s\" on host \"%s\" is unavailable.\n",
                service,
                host_or_ip);
        break;
      }
      case UNKNOWN_HOST: {
        fprintf(stderr, "Lookup for host \"%s\" is failed.\n", host_or_ip);
        break;
      }
      case UNKNOWN_SERVICE: {
        fprintf(stderr, "Service \"%s\" is not well known\n", service);
        break;
      }
      default: {
        fprintf(stderr, "Unknown error %d\n", service_state);
        break;
      }
    }
    // host + port
  } else if (strlen(host_or_ip) != 0 && port != 0) {
    service_state = host_port_probe(host_or_ip, port, NULL);

    switch (service_state) {
      case AVAILABLE: {
        printf("Port \"%u\" on host \"%s\" is available.\n", port, host_or_ip);
        break;
      }
      case UNAVAILABLE: {
        fprintf(stderr,
                "Port \"%u\" on host \"%s\" is unavailable.\n",
                port,
                host_or_ip);
        break;
      }
      case UNKNOWN_HOST: {
        fprintf(stderr, "Lookup for host \"%s\" is failed.\n", host_or_ip);
        break;
      }
      case UNKNOWN_SERVICE: {
        fprintf(stderr, "Service \"%s\" is not well known\n", service);
        break;
      }
      default: {
        fprintf(stderr, "Unknown error %d\n", service_state);
        break;
      }
    }
  } else {
    fputs("Not enough arguments.\n", stderr);
    exit(EXIT_FAILURE);
  }

  if (service_state != AVAILABLE) {
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}
