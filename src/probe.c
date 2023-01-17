#include "probe.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static probe_conf_t probe_conf = { .retry_count = DEFAULT_RETRY_COUNT,
                                   .timeout = DEFAULT_TIMEOUT };

#ifdef DEBUG

static void
print_config()
{
  puts("DEBUG \"print_config\" {");
  printf("\tRetry count: %d\n", probe_conf.retry_count);
  printf("\tTimeout: %d\n", probe_conf.timeout);
  puts("}");
}

static void
print_protoent(struct protoent* proto_ent)
{
  assert(proto_ent != NULL);

  puts("DEBUG \"print_protoent\" {");

  printf("\tOfficial protocol name(p_name): %s\n", proto_ent->p_name);
  printf("\tProtocol number(p_proto): %d\n", proto_ent->p_proto);

  puts("}");
}

static void
print_servent(struct servent* serv_ent)
{
  assert(serv_ent != NULL);
  char** s_aliases = serv_ent->s_aliases;

  puts("DEBUG \"print_servent\" {");

  printf("\tOfficial service name(s_name): %s\n", serv_ent->s_name);

  puts("\tAlias list:");

  while (*s_aliases != NULL) {
    printf("\tAlias: %s\n", *s_aliases++);
  }

  printf("\tPort number(s_port): %d\n", ntohs(serv_ent->s_port));
  printf("\tProtocol to use(s_proto): %s\n", serv_ent->s_proto);

  puts("}");
}

static void
print_sockaddr_in(struct sockaddr_in* p_addr)
{
  assert(p_addr != NULL);
  // only PF_INET
  assert(p_addr->sin_family == PF_INET);

  puts("DEBUG \"print_sockaddr_in\" {");

  printf("\tInternet address(sin_addr): %s\n", inet_ntoa(p_addr->sin_addr));
  printf("\tPort number(sin_port): %d\n", ntohs(p_addr->sin_port));

  puts("}");
}

static void
print_hostent(struct hostent* p_host)
{
  assert(p_host != NULL);
  struct in_addr addr;
  in_addr_t** p_next_addr = (in_addr_t**)p_host->h_addr_list;
  size_t i = 0ull;

  puts("DEBUG \"print_hostent\" {");

  printf("\tOfficial name of host(h_name): %s\n", p_host->h_name);

  printf("\tHost address type(h_addrtype): ");

  switch (p_host->h_addrtype) {
    case PF_INET: {
      puts("\tIP protocol family");
      break;
    }
    default: {
      printf("\tProtocol family: %d\n", p_host->h_addrtype);
      return;
    }
  }

  puts("\tAlias list:");

  while (*p_next_addr != NULL) {
    addr.s_addr = **p_next_addr;
    printf("\tAddress: %s\n", inet_ntoa(addr));
    p_next_addr++;
  }

  puts("}");
}

#endif // DEBUG

static SERVICE_STATE
probe(struct sockaddr_in* sockaddr, struct protoent* proto)
{
#ifdef DEBUG
  print_sockaddr_in(sockaddr);
#endif

  socket_t sock;

  switch (proto->p_proto) {
    case (IPPROTO_TCP): {
      sock = socket(PF_INET, SOCK_STREAM, proto->p_proto);
      break;
    }
    default: {
      perror("Unsupported protocol in `probe` call");
      exit(EXIT_FAILURE);
    }
  }

  if (sock == -1) {
    perror("Socket creation in `probe` call");
    exit(EXIT_FAILURE);
  }

  int conn_res;

  for (size_t i = 0; i < probe_conf.retry_count; ++i) {
#ifdef DEBUG
    printf("Trying to connect %d time\n", i);
#endif

    conn_res = connect(sock, (struct sockaddr*)sockaddr, INET_ADDRSTRLEN);

    if (conn_res == 0) {
      break;
    } else if (i != probe_conf.retry_count - 1) {
      sleep(probe_conf.timeout);
    }
  }

  close(sock);

  if (conn_res == 0) {
    return AVAILABLE;
  }

#ifdef DEBUG
  perror("Connect creation in `probe` call");
#endif

  return UNAVAILABLE;
}

void
probe_config(size_t retry_count, size_t timeout)
{
  probe_conf.retry_count = retry_count;
  probe_conf.timeout = timeout;
}

char*
probe_version()
{
  return PROBE_VERSION;
}

SERVICE_STATE
ipv4_port_probe(char* ipv4, in_port_t port, char* protocol)
{
  if (protocol == NULL) {
    protocol = DEFAULT_SERVICE_PROTOCOL;
  }

  struct protoent* proto = getprotobyname(DEFAULT_SERVICE_PROTOCOL);
  struct hostent* host_ent;
  struct sockaddr_in sockaddr;
  struct in_addr addr;

  if (inet_aton(ipv4, &addr) == 0) {
    return INVALID_IP;
  }

  if (proto == NULL) {
#ifdef DEBUG
    fprintf(stderr, "Unknown protocol: %s\n", protocol);
#endif
    return UNKNOWN_PROTOCOL;
  }

#ifdef DEBUG
  print_config();
  print_protoent(proto);
#endif

  host_ent = gethostbyaddr(&addr, sizeof(struct in_addr), AF_INET);

  if (host_ent == NULL) {
    switch (h_errno) {
      case HOST_NOT_FOUND: {
#ifdef DEBUG
        fprintf(stderr, "Host by ip \"%s\" is not found\n", ipv4);
#endif
        return UNKNOWN_HOST;
      }
      case TRY_AGAIN: {
#ifdef DEBUG
        fprintf(
          stderr, "Host by ip \"%s\" is not found. Try again later\n", ipv4);
#endif
        return UNAVAILABLE;
      }
      case NO_RECOVERY: {
#ifdef DEBUG
        fprintf(stderr, "Non recoverable error\n");
#endif
        return UNAVAILABLE;
      }
      case NO_DATA: {
#ifdef DEBUG
        fprintf(stderr, "No data record of requested ip \"%s\"\n", ipv4);
#endif
        return UNAVAILABLE;
      }
    }
  }

#ifdef DEBUG
  print_hostent(host_ent);
#endif

  if (host_ent->h_addrtype != AF_INET) {
    fprintf(stderr, "Only IPv4 is supported\n");
    exit(EXIT_FAILURE);
  }

  sockaddr.sin_addr = addr;
  sockaddr.sin_family = PF_INET;
  sockaddr.sin_port = htons(port);

  return probe(&sockaddr, proto);
}

SERVICE_STATE
ipv4_service_probe(char* ipv4, char* service, char* protocol)
{
  if (protocol == NULL) {
    protocol = DEFAULT_SERVICE_PROTOCOL;
  }

  struct protoent* proto = getprotobyname(DEFAULT_SERVICE_PROTOCOL);
  struct hostent* host_ent;
  struct servent* serv_ent = getservbyname(service, DEFAULT_SERVICE_PROTOCOL);
  struct sockaddr_in sockaddr;
  struct in_addr addr;

  if (inet_aton(ipv4, &addr) == 0) {
    return INVALID_IP;
  }

  if (proto == NULL) {
#ifdef DEBUG
    fprintf(stderr, "Unknown protocol: %s\n", protocol);
#endif
    return UNKNOWN_PROTOCOL;
  }

#ifdef DEBUG
  print_config();
  print_protoent(proto);
#endif

  host_ent = gethostbyaddr(&addr, sizeof(struct in_addr), AF_INET);

  if (host_ent == NULL) {
    switch (h_errno) {
      case HOST_NOT_FOUND: {
#ifdef DEBUG
        fprintf(stderr, "Host by ip \"%s\" is not found\n", ipv4);
#endif
        return UNKNOWN_HOST;
      }
      case TRY_AGAIN: {
#ifdef DEBUG
        fprintf(
          stderr, "Host by ip \"%s\" is not found. Try again later\n", ipv4);
#endif
        return UNAVAILABLE;
      }
      case NO_RECOVERY: {
#ifdef DEBUG
        fprintf(stderr, "Non recoverable error\n");
#endif
        return UNAVAILABLE;
      }
      case NO_DATA: {
#ifdef DEBUG
        fprintf(stderr, "No data record of requested ip \"%s\"\n", ipv4);
#endif
        return UNAVAILABLE;
      }
    }
  }

#ifdef DEBUG
  print_hostent(host_ent);
#endif

  if (host_ent->h_addrtype != AF_INET) {
    fprintf(stderr, "Only IPv4 is supported\n");
    exit(EXIT_FAILURE);
  }

  if (serv_ent == NULL) {
#ifdef DEBUG
    fprintf(stderr, "Invalid service: %s\n", service);
#endif
    return UNKNOWN_SERVICE;
  }

#ifdef DEBUG
  print_servent(serv_ent);
#endif

  sockaddr.sin_addr = addr;
  sockaddr.sin_family = PF_INET;
  sockaddr.sin_port = serv_ent->s_port;

  return probe(&sockaddr, proto);
}

SERVICE_STATE
host_service_probe(char* host, char* service, char* protocol)
{
  if (protocol == NULL) {
    protocol = DEFAULT_SERVICE_PROTOCOL;
  }

  struct protoent* proto = getprotobyname(protocol);
  struct hostent* host_ent = gethostbyname(host);
  struct servent* serv_ent = getservbyname(service, protocol);
  struct sockaddr_in sockaddr;

  if (proto == NULL) {
#ifdef DEBUG
    fprintf(stderr, "Unknown protocol: %s\n", protocol);
#endif
    return UNKNOWN_PROTOCOL;
  }

#ifdef DEBUG
  print_config();
  print_protoent(proto);
#endif

  if (host_ent == NULL) {
#ifdef DEBUG
    fprintf(stderr, "Invalid host: %s\n", host);
#endif
    return UNKNOWN_HOST;
  }

  if (serv_ent == NULL) {
#ifdef DEBUG
    fprintf(stderr, "Invalid service: %s\n", service);
#endif
    return UNKNOWN_SERVICE;
  }

#ifdef DEBUG
  print_hostent(host_ent);
#endif

  if (host_ent->h_addrtype != AF_INET) {
    fprintf(stderr, "Only IPv4 is supported\n");
    exit(EXIT_FAILURE);
  }

#ifdef DEBUG
  print_servent(serv_ent);
#endif

  // TODO: check all addresses
  sockaddr.sin_addr = *(struct in_addr*)host_ent->h_addr;
  sockaddr.sin_family = PF_INET;
  sockaddr.sin_port = serv_ent->s_port;

  return probe(&sockaddr, proto);
}

SERVICE_STATE
host_port_probe(char* host, in_port_t port, char* protocol)
{
  if (protocol == NULL) {
    protocol = DEFAULT_SERVICE_PROTOCOL;
  }

  struct protoent* proto = getprotobyname(protocol);
  struct hostent* host_ent = gethostbyname(host);
  struct sockaddr_in sockaddr;

  if (proto == NULL) {
#ifdef DEBUG
    fprintf(stderr, "Unknown protocol: %s\n", protocol);
#endif
    return UNKNOWN_PROTOCOL;
  }

#ifdef DEBUG
  print_config();
  print_protoent(proto);
#endif

  if (host_ent == NULL) {
#ifdef DEBUG
    fprintf(stderr, "Invalid host: %s\n", host);
#endif
    return UNKNOWN_HOST;
  }

#ifdef DEBUG
  print_hostent(host_ent);
#endif

  if (host_ent->h_addrtype != AF_INET) {
    fprintf(stderr, "Only IPv4 is supported\n");
    exit(EXIT_FAILURE);
  }

  // TODO: check all addresses
  sockaddr.sin_addr = *(struct in_addr*)host_ent->h_addr;
  sockaddr.sin_family = PF_INET;
  sockaddr.sin_port = htons(port);

  return probe(&sockaddr, proto);
}
