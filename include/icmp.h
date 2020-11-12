#pragma once

#include "skel.h"
#include "utils.h"

#include <utility>

class IcmpHandler
{
public:
  /*
   * Receives an ICMP echo request packet and returns an echo reply.
   */
  static int handle_echo(packet *p);

  /*
   * Sends the ICMP error identified by type and code to the given host.
   */
  static int send_errors(packet *p, uint8_t type, uint8_t code);
};
