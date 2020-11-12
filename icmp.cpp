#include <icmp.h>
#include <netinet/if_ether.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>

int IcmpHandler::handle_echo(packet *p)
{
  ethhdr *eth = (ethhdr*) p->payload;
  iphdr *ip_hdr = (iphdr *) (p->payload + sizeof(ethhdr));
	icmphdr *icmp_hdr = (icmphdr *) (p->payload + sizeof(ethhdr) + sizeof(iphdr));

  uint8_t temp_mac[6];
  memcpy(temp_mac, eth->h_source, 6);
  get_interface_mac(p->interface, eth->h_source);
  memcpy(eth->h_dest, temp_mac, 6);

  // Update the iphdr fields
  std::swap(ip_hdr->saddr, ip_hdr->daddr);
  ip_hdr->ttl--;
  ip_hdr->check = 0;
  ip_hdr->check = checksum(ip_hdr, sizeof(iphdr));

  icmp_hdr->type = ICMP_ECHOREPLY;

  // Compute the checksum and update the corresponding field.
  icmp_hdr->checksum = 0;
  icmp_hdr->checksum = checksum(icmp_hdr, sizeof(icmphdr));

  send_packet(p->interface, p);
}

int IcmpHandler::send_errors(packet *p, uint8_t type, uint8_t code)
{
  ethhdr *eth = (ethhdr *) p->payload;
  iphdr *ip_hdr = (iphdr *) (p->payload + sizeof(ethhdr));
	icmphdr *icmp_hdr = (icmphdr *) (p->payload + sizeof(ethhdr) + sizeof(iphdr));
  memcpy(p->payload + sizeof(ethhdr) + sizeof(iphdr) + sizeof(icmphdr), ip_hdr, sizeof(iphdr) + 8);

  memcpy(eth->h_dest, eth->h_source, 6);
  get_interface_mac(p->interface, eth->h_source);

  // Update the IP header.
  ip_hdr->daddr = ip_hdr->saddr;
  inet_pton(AF_INET, get_interface_ip(p->interface), (void *) &ip_hdr->saddr);
  ip_hdr->protocol = IPPROTO_ICMP;
  ip_hdr->ttl = MAXTTL;
  ip_hdr->tot_len = htons(2 * sizeof(iphdr) + sizeof(icmphdr) + 8);

  icmp_hdr->type = type;
  icmp_hdr->code = code;

  // Update the checksum in the icmphdr.
  icmp_hdr->checksum = 0;
  icmp_hdr->checksum = checksum(icmp_hdr, sizeof(icmphdr) + sizeof(iphdr) + 8);
  ip_hdr->check = 0;
  ip_hdr->check = checksum(ip_hdr, sizeof(iphdr));

  // Set the packet length.
  p->len = sizeof(ethhdr) + 2 * sizeof(iphdr) + sizeof(icmphdr) + 8;

  send_packet(p->interface, p);
}
