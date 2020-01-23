/* Author: Diyo Davis */
/* Title : Program to extract files from FTP pcap file*/

#include<pcap/pcap.h>
#include<net/ethernet.h>
#include<linux/ip.h>
#include<linux/tcp.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<fcntl.h>
#include<unistd.h>

struct ether_header * eth = NULL;
struct iphdr * ip = NULL;
struct tcphdr * tcp = NULL;

char *fname, *payload_offset = NULL;
int count, flag, fd, ret, pkt_len, payload_len = 0;
int hdr_len = sizeof(struct ether_header) + sizeof(struct iphdr);
unsigned long size, wr = 0;

void process_packet(u_char * user, const struct pcap_pkthdr * h, const u_char * bytes){
	eth = (struct ether_header *)(bytes);
	if(ntohs(eth->ether_type) != 2048)	return;

	ip = (struct iphdr *)(bytes + sizeof(struct ether_header));
	if(ip->protocol != 6)	return;

	tcp = (struct tcphdr *)(bytes + sizeof(struct ether_header) + sizeof(struct iphdr));

	pkt_len = ntohs(ip->tot_len) - sizeof(struct iphdr);
	payload_len = h->caplen - (hdr_len + 4 * tcp->doff);
	payload_offset = bytes + hdr_len + 4 * tcp->doff;

	if(!strncmp("SIZE", payload_offset, 4)){
		size = 1;
		return;
	}

	if(!strncmp("213", payload_offset, 3) && size == 1)
		size = strtoul(payload_offset + 4, NULL, 10);

	if(!strncmp("RETR", payload_offset, 4)){
		payload_len = payload_len - 7;
		
		fname = (char *)malloc(payload_len);
		memset(fname, '\0', payload_len);
		strncpy(fname, payload_offset + 5, payload_len);
		if(strrchr(fname, '/') == NULL)
			fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0777);
		else	fd = open(strrchr(fname, '/') + 1, O_CREAT | O_WRONLY | O_TRUNC, 0777);
		memset(fname, '\0', payload_len);
		free(fname);
		
		ret = 1;
		return;
	}
	
	if(flag == 1 && payload_len != 0){
		ret = write(fd, payload_offset, payload_len);
		wr += ret;
		if(wr == size){
			flag = ret = wr = 0;
			close(fd);
		}
	}

	if((!strncmp("150", payload_offset, 3)) && (ret == 1)){
		flag = 1;
		ret = 0;
		return;
	}
}

int main(int argc, char ** argv){
	if(argc == 1)
		printf("No pcap file specified.\n");
	else if(argc > 2)
		printf("Specify only one pcap file at a time.\n");
	else{
		char errbuf[PCAP_ERRBUF_SIZE];
		pcap_t *handle = NULL;
		handle = pcap_open_offline(argv[1], errbuf);
		if(handle == NULL)
			printf("%s : File not found\n",argv[1]);
		else
			pcap_loop(handle, -1, process_packet, NULL);
	}
	return 0;
}