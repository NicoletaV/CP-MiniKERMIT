/* VINA NICOLETA 325CD */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

/* formeaza si returneaza un pachet, in functie de tipul pachetului primit */
pkg* get_pkg(char seq, char type, char* data, int data_len) {

	pkg *package = (pkg *) malloc(sizeof(pkg));
	package->SOH = 0x01;
	package->LEN = 5 + data_len;
	package->SEQ = (seq + 1) % 64;
	package->MARK = 0x0D;
	package->TYPE = type;
	memset(package->DATA, 0, 250); 

	if(type == F || type == D) {

		memcpy(package->DATA, data, data_len);
	}

	package->CHECK = crc16_ccitt(package, 4 + data_len);

	return package;
}

/* formeaza si returneaza un mesaj, in functie de tipul type primit */
msg* get_msg(char seq, char type, char* data, int data_len) {

    pkg *p = get_pkg(seq, type, data, data_len);

    msg* aux = (msg*)malloc(sizeof(msg));    
	memcpy(aux->payload, p, sizeof(pkg));
	aux->len = data_len + 5;

    return aux;
}

/* trimite de maxim 3 ori un mesaj, altfel se opreste (timeout) */
void verify_timeout(msg **t, msg **r) {

	int timeout_counter = 3;

	send_message(*t);
	
	int aux = (*t)->payload[2] + 1;

	/* pentru actualizarea check-ului pentru NAK */
	unsigned char c = (*t)->payload[1] - 1;
	unsigned short *crc = (unsigned short *)&(*t)->payload[254];

	while(1) {		

		if(timeout_counter == 0) {
			exit(0);
		}		

		*r = receive_message_timeout(5000);

		if (*r == NULL) {

			send_message(*t);
			timeout_counter--;

		} else {

			if((*r)->payload[2] != aux) {
				continue;
			}
			
			if((*r)->payload[3] == Y) {
				return;

			} else {
				
				(*t)->payload[2] = (char)((*r)->payload[2] + 1) % 64;
				*crc = crc16_ccitt((*t)->payload, c);
				send_message(*t);
				aux = (aux + 2) % 64;				
			}
		}
	}
}


int main(int argc, char** argv) {

	msg *at =(msg*) malloc(sizeof(msg)), *ar;
	int i;

	init(HOST, PORT);

	data_S pkg_type_S;
	pkg_type_S.MAXL = 250;
	pkg_type_S.TIME = 5;
	pkg_type_S.NPAD = 0x00;
	pkg_type_S.PADC = 0x00;
	pkg_type_S.EOL = 0x0D;
	pkg_type_S.QCTL = 0x00;
	pkg_type_S.QBIN = 0x00;
	pkg_type_S.CHKT = 0x00;
	pkg_type_S.REPT = 0x00;
	pkg_type_S.CAPA = 0x00;
	pkg_type_S.R = 0x00;

	pkg p;
	p.SOH = 0x01;
	p.LEN = 16;
	p.SEQ = 0x00;
	p.TYPE = S;
	
	memset(p.DATA, 0, 250);
	memcpy(p.DATA, &pkg_type_S, 11);

	p.CHECK = crc16_ccitt(&p, 15);
	p.MARK = pkg_type_S.EOL;

	memcpy(at->payload, &p, 257);
	at->len = 11;

	/* se trimite pachetul de tip S */
	verify_timeout(&at, &ar);

	int sequence = ar->payload[2];

	/* pentru fiecare fiser de intrare */
	for(i = 1; i < argc; i++) {

		at = get_msg(sequence, F, argv[i], strlen(argv[i]));

		/* se trimite pachet File header */
		verify_timeout(&at, &ar);

	   	FILE *file = fopen(argv[i], "rb");

	    if(file == NULL) {
	        perror("Can't open file\n");
	        exit(0);
	    }

	    char buf[250];
	    int read_no;

	    while(1) {

	    	memset(buf, 0, 250);
	    	read_no = fread(buf, sizeof(char), 249, file);

    		sequence = ar->payload[2];

	    	/* daca se citesc 0 elemente, s-a ajuns la EOF si trimite mesaj 
	    	de stip Z; inchide fisierul */
	    	if(read_no == 0) { 

	    		at = get_msg(sequence, Z, NULL, 0);

	    		verify_timeout(&at, &ar);

	    		fclose(file);
				
				break;
	    	} 
			
			/* formeaza pachet de tip Data si il tirmite */
			at = get_msg(sequence, D, buf, read_no);
		
	    	verify_timeout(&at, &ar);
	    }

	    sequence = ar->payload[2];	   
	}

	/* formeaza pachet EOT si il trimite */
	at = get_msg(sequence, B, NULL, 0);
	
	verify_timeout(&at, &ar);
	
	return 0;
}
