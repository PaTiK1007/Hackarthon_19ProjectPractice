#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#include <wiringPi.h>



/*
* DHT sensor -> read temperat and print
* we need to pass the data to http server.
*/

int DHTPIN = 7 ;
char* read_dht11 () {
  int d[5] = {0, 0, 0, 0, 0} ;
  uint8_t i, j = 0 ;
  uint8_t counter = 0 ;
	char* result = (char*) malloc(sizeof(char) * 32);

  pinMode(DHTPIN, OUTPUT) ;
  digitalWrite(DHTPIN, LOW ) ;
  delay(18) ;
	  //digitalWrite(DHTPIN, HIGH) ;
	  //delayMicroseconds(40) ;

  digitalWrite(DHTPIN, HIGH) ;
  uint8_t lastState = HIGH ;

  delayMicroseconds(20) ;

  pinMode(DHTPIN, INPUT) ;

  for (i = 0 ; i < 85 ; i++) {
    counter = 0 ;
    while (digitalRead(DHTPIN) == lastState) {
      counter++ ;
      delayMicroseconds(1) ;
      if (counter == 255)
        break ;
    }
    lastState = digitalRead(DHTPIN) ;

    if (counter == 255) {
       printf(".") ;
       break ;
    }
    if (i >= 4 && i % 2 == 0) {
      d[j/8] <<= 1 ;
      if (counter > 30)
	d[j/8] |= 1 ;
      j++ ;
    }
  }

  if (j >= 40 && (d[4] == ((d[0] + d[1] + d[2] + d[3]) & 0xFF))) {
    float f ;
    f = d[2] * 9.0 / 5.0 + 32 ;
    printf("Humidity = %d.%d\t", d[0], d[1]) ;
    printf("Temperat = %d.%d C\n", d[2], d[3]) ;
		sprintf(result, "%d.%d", d[2], d[3]);
		return result;
    //printf("Farenhei = %f\n", f) ;
  }
  else {
    printf("data not good\n") ;
		result = strdup("data not good\n");
    // printf("Humidity = %d.%d\t", d[0], d[1]) ;
    // printf("Temperat = %d.%d C\n", d[2], d[3]) ;
  }
}

void * worker (void * arg) {
	int conn ;
	char buf[1024] ;
	char * data = 0x0, * orig = 0x0 ;
	int len = 0 ;
	int s ;

	conn = *((int *)arg) ;
	free(arg) ;

	/*
	* make a string to send
	*/
	data = strdup("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Lenght:19\r\n\r\n<html>");
	strcat(data, read_dht11 ());
	strcat("</html>\r\n\r\n");

	// data = strdup("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Lenght:19\r\n\r\n<html>hello</html>\r\n\r\n") ;
	len = strlen(data) ;

	orig = data ;
	while (len > 0 && (s = send(conn, data, len, 0)) > 0) {
		data += s ;
		len -= s ;
	}
	shutdown(conn, SHUT_WR) ;
	if (orig != 0x0)
		free(orig) ;

	return 0x0 ;
}

int main (int argc, char const *argv[]) {
	int listen_fd, new_socket ;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	if (argc != 2) {
		fprintf(stderr, "Wrong number of arguments") ;
		exit(EXIT_FAILURE) ;
	}

	char buffer[1024] = {0};

	listen_fd = socket(AF_INET /*IPv4*/, SOCK_STREAM /*TCP*/, 0 /*IP*/) ;
	if (listen_fd == 0)  {
		perror("socket failed : ");
		exit(EXIT_FAILURE);
	}

	memset(&address, '0', sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY /* the localhost*/ ;
	address.sin_port = htons(atoi(argv[1]));

	if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed : ");
		exit(EXIT_FAILURE);
	}

	while (1) {
		if (listen(listen_fd, 16 /* the size of waiting queue*/) < 0) {
			perror("listen failed : ");
			exit(EXIT_FAILURE);
		}

		new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen) ;
		if (new_socket < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		pthread_t worker_tid ;
		int * worker_arg = (int *) malloc(sizeof(int)) ;
		*worker_arg = new_socket ;

		pthread_create(&worker_tid, 0x0, worker, (void *) worker_arg) ;
	}
}
