#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "../ssocket/ssocket.h"

//Definiciones
#define BUF_SIZE 10
#define FIRST_PORT 1820

//Variables
int first_pack = 0;
struct timeval dateInicio, dateFin;
char buf[BUF_SIZE];
char* IP_DEST;
int mostrarInfo = 0;
int MAX_PACKS = 1;
int DESTINATION_PORT = FIRST_PORT;
double segundos;

/*
	Esta version del cliente recibe: 
		-cantidad de paquetes a enviar
		-ip de destino
		-puerto de destino
*/

main(int argc, char **argv) {

	if(argc < 4){
		fprintf(stderr, "Syntax Error: Esperado: ./client MAX_PACKS IP_DEST DESTINATION_PORT\n");
		exit(1);
	}

	//Recuperar total de paquetes a enviar
	MAX_PACKS = atoi(argv[1]);

	//Recuperar IP destino
	IP_DEST = argv[2];

	//Recuperar total de puertos
	DESTINATION_PORT = atoi(argv[3]);

	/* Llenar de datos el buffer a enviar */
	int i;
	for(i = 0; i < BUF_SIZE; i++)
		buf[i] = 'h'+i;

	//Crear Socket
	int socket_fd;
	char ports[10];
	sprintf(ports, "%d", DESTINATION_PORT);
	socket_fd = udp_connect(IP_DEST, ports);
	if(socket_fd < 0) {
		fprintf(stderr, "connection refused\n");
		exit(1);
	}


	//Medir Inicio
	gettimeofday(&dateInicio, NULL);

	for(i = 0; i < MAX_PACKS; i++){
		if(write(socket_fd, buf, BUF_SIZE) != BUF_SIZE) {
			gettimeofday(&dateFin, NULL);
			segundos = (dateFin.tv_sec+dateFin.tv_usec/1000000.)-(dateInicio.tv_sec*1.0+dateInicio.tv_usec/1000000.);
			//fprintf(stderr, "Falla el write al servidor, envio %d paquetes\n", i);
			//fprintf(stderr, "total time = %g\n", segundos);
			break;
		}
	}

	//Medir Fin
	gettimeofday(&dateFin, NULL);

	segundos=(dateFin.tv_sec*1.0+dateFin.tv_usec/1000000.)-(dateInicio.tv_sec*1.0+dateInicio.tv_usec/1000000.);
	if(mostrarInfo){
		printf("Tiempo Total = %g\n", segundos);
		printf("QPS = %g\n", MAX_PACKS*1.0/segundos);
	}else{
		printf("%g \n", segundos);
	}
	exit(0);
}