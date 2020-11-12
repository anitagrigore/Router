#include <netinet/if_ether.h>
#include <netinet/in.h>
#include "skel.h"
#include "arp.h"
#include "icmp.h"
#include "utils.h"
#include "routing.h"

#include <iostream>
#include <list>

bool process_packet(packet *m, ArpTable &at, RoutingTable &rt, std::list<packet> &packets_queue,
	bool already_in_queue);

int main(int argc, char *argv[])
{
	packet m;
	int rc;
	bool has_error = false;
	RoutingTable rt = RoutingTable::from_file("rtable.txt", has_error);
	std::list<packet> packets_queue;

	init();

	ArpTable arp_table;

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");

		ethhdr *eth = (ethhdr*) m.payload;
		std::cerr << "eth->h_proto = " << ntohs(eth->h_proto) << std::endl;

		// If the protocol is ARP, extract the arphdr and treat the requests and replies.
		if(eth->h_proto == htons(ETH_P_ARP)) {
			std::cerr << "arp" << std::endl;

			arphdr *arp = (arphdr*) (m.payload + sizeof(ethhdr));
			if(arp->ar_op == htons(ARPOP_REQUEST)) {
				ARP::handle_request(&m);
			} else if (arp->ar_op == htons(ARPOP_REPLY)) {
				std::cerr << "arp reply";
				arp_table.handle_reply(&m);

				for (auto it = packets_queue.begin(); it != packets_queue.end();) {
					bool remove_from_queue = process_packet(&(*it), arp_table, rt, packets_queue, true);

					if (remove_from_queue) {
						auto next = std::next(it);
						packets_queue.erase(it);
						it = next;
					} else {
						it++;
					}
				}
			}
		}

		// If the protocol is IP, extract the iphdr.
		if(eth->h_proto == htons(ETH_P_IP)) {
			iphdr *ip_hdr = (iphdr *)(m.payload + sizeof(ethhdr));

			if (checksum(ip_hdr, sizeof(iphdr)) != 0) {
				continue;
			}

			uint32_t my_ip;
			inet_pton(AF_INET, get_interface_ip(m.interface), &my_ip);

			// If the packet is not destined for the router, forward it.
			if (my_ip != ip_hdr->daddr) {
				process_packet(&m, arp_table, rt, packets_queue, false);
			} else if(ip_hdr->protocol == IPPROTO_ICMP) {
				// Extract the ICMP header.
				icmphdr *icmp_hdr = (icmphdr *)(m.payload + sizeof(ethhdr) + sizeof(iphdr));

				if(icmp_hdr->type == ICMP_ECHO) {
					IcmpHandler::handle_echo(&m);
				}
			}
		}
	}
}

// Returns true if the packet has been processed, false otherwise
bool process_packet(packet *m, ArpTable &at, RoutingTable &rt, std::list<packet> &packets_queue,
	bool already_in_queue)
{
	ethhdr *eth = (ethhdr *) m->payload;
	iphdr *ip_hdr = (iphdr *)(m->payload + sizeof(ethhdr));

	// Handle the errors.
	if(ip_hdr->ttl <= 1) {
		IcmpHandler::send_errors(m, ICMP_TIME_EXCEEDED, 0);
		return true;
	}

	auto [route, has_route] = rt.get_route(ip_hdr->daddr);
	if(!has_route) {
		IcmpHandler::send_errors(m, ICMP_DEST_UNREACH, 0);
		return true;
	}

	// Extract the MAC address of the next_hop.
 	auto [mac, has_mac] = at.get_mac(route.next_hop);

	// If the MAC is known, forward the packet.
	if (has_mac) {
		ip_hdr->ttl--;
		ip_hdr->check = 0;
		ip_hdr->check = checksum(ip_hdr, sizeof(iphdr));

		std::copy(mac.begin(), mac.end(), std::begin(eth->h_dest));

		send_packet(route.interface, m);

		return true;
	}

	// If the packet is not in queue, send a request for the next_hop.
	if(!already_in_queue) {
		packets_queue.push_back(*m);
		ArpTable::send_request(route.next_hop.data(), route.interface);
	}

	return false;
}
