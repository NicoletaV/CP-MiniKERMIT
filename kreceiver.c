/* VINA NICOLETA 325CD */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

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

/* compara vechiul crc cu noul crc calculat */
int check(msg *r) {

    unsigned short *aux = (unsigned short *) &(r->payload[254]);
    unsigned char c = r->payload[1] - 1;

    return (crc16_ccitt(r->payload, c) == *aux);
}

/* retrimite de maxim 3 ori un mesaj */
void wait_message(msg **t, msg **r, int dec) {
    
    int timeout_counter = 3;

    while(1) {

        if(timeout_counter == 0) {
            exit(0);
        }

        *r = NULL;

        if(dec) {
            *r = receive_message_timeout(15000);
        } else {
            *r = receive_message_timeout(5000);
        }        

        if (*r == NULL) {

            if(!dec) {

                send_message(*t);
            }

            timeout_counter--;
                           
        } else {

            if(check(*r)) {

                *t = get_msg((*r)->payload[2], Y, NULL, 0);

                send_message(*t);          

                return;

            } else if(!dec){

                *t = get_msg((*r)->payload[2], N, NULL, 0);
          
                send_message(*t);               
            }
        }
    }   
}


int main(int argc, char** argv) {

    msg *at, *ar;

    init(HOST, PORT);

    /* se primeste pachetul de tip S */
    wait_message(&at, &ar, 1);

    while(1) {

        wait_message(&at, &ar, 0);

        /* cand primeste EOT, iese si programul se incheie */
        if(ar->payload[3] == B) {
            break;
        }

        /* se formeaza numele fisierului curent si se deschide */
        char *recv_filename = (char *) malloc(6 + strlen(&ar->payload[4]));
        strcpy(recv_filename, "recv_\0");

        strcat(recv_filename, &ar->payload[4]);

        FILE *file = fopen(recv_filename, "wb");

        if(file == NULL) {
            perror("Can't create recv_file\n");
            exit(0);
        }        

        while(1) {

            wait_message(&at, &ar, 0);

            /* daca primeste EOF, se inchide fisierul si se trece la urmatorul */
            if(ar->payload[3] == Z) {

                fclose(file);

                break;

            } else if(ar->payload[3] == D) {

                /* se formeaza in aux bucata de date primita si acesta este scris 
                in fisier; in (ar->len - 5) se afla doar lungimea campului data */
                char *aux = calloc(1, ar->len - 5);
                memcpy(aux, &ar->payload[4], ar->len - 5);

                fwrite(aux, sizeof(char), ar->len - 5, file);

            } else {
                printf("Pachetul nu e valid\n");
            }
        }
    }
	
	return 0;
}
