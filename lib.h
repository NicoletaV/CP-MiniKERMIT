#ifndef LIB
#define LIB

#define S 0
#define F 1
#define D 2
#define Z 3
#define B 4
#define Y 5
#define N 6
#define E 7

typedef struct {
    int len;
    char payload[1400];
} msg;

typedef struct {
	unsigned char SOH;
	unsigned char LEN;
	unsigned char SEQ;
	unsigned char TYPE;
	unsigned char DATA[250];
	unsigned short CHECK;
	unsigned char MARK;
} __attribute__ ((__packed__)) pkg;

typedef struct {
	unsigned char MAXL;
	unsigned char TIME;
	unsigned char NPAD;
	unsigned char PADC;
	unsigned char EOL;
	unsigned char QCTL;
	unsigned char QBIN;
	unsigned char CHKT;
	unsigned char REPT;
	unsigned char CAPA;
	unsigned char R;
} __attribute__ ((__packed__)) data_S;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

#endif

