#define _GNU_SOURCE

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "../ssocket/ssocket.h"

#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>

//Definiciones
#define BUF_SIZE 10
#define FIRST_PORT 1820

//Variables
int first_pack = 0;
struct timeval dateInicio, dateFin;
pthread_mutex_t lock;
int mostrarInfo = 1;
int distribuiteCPUs = 1;
int MAX_PACKS = 1;
int NTHREADS = 1;
int DESTINATION_PORT = FIRST_PORT;
double segundos;

// Variables para Ftrace
int trace_fd = -1;
char *trace_path = "/sys/kernel/debug/tracing/tracing_on";
int pid_trace_fd = -1;
char *pid_trace_path = "/sys/kernel/debug/tracing/set_ftrace_pid";
//originalmente era "no pid"
int marker_fd = -1;
char *marker_path = "/sys/kernel/debug/tracing/trace_marker";

/*	Esta version del servidor recibe: 
		-cantidad de paquetes a enviar
		-Numero de Threads a lanzar para compartir el socket
		-puerto de bind para el socket* 	*/

llamadaHilo(int socket_fd){
	char buf[BUF_SIZE];
	int lectura;

	int actualCPU = sched_getcpu();
	if(mostrarInfo) printf("Socket Operativo: %d, \t CPU: %d\n", socket_fd, actualCPU);

	int i;
	int paquetesParaAtender = MAX_PACKS/NTHREADS;

		// Marca
		write(marker_fd, "MITRACE: Nuevo Thread\n", 22);

	for(i = 0; i < paquetesParaAtender; i++) {
		//lectura = recv(socket_fd, buf, BUF_SIZE, 0);

			// Marca
			write(marker_fd, "MITRACE: Comienza el read del socket\n", 37);
		lectura = read(socket_fd, buf, BUF_SIZE);
		if(lectura <= 0) {
			fprintf(stderr, "Error en el read del socket (%d)\n", lectura);
			exit(1);
		}
		if(first_pack==0) { 
			pthread_mutex_lock(&lock);
			if(first_pack == 0) {
				if(mostrarInfo)	printf("got first pack\n");
				first_pack = 1;
				//Medir Inicio
				gettimeofday(&dateInicio, NULL);
			}
			pthread_mutex_unlock(&lock);
		}
	}

	if(mostrarInfo) printf("Fin Socket Operativo: %d, \t CPU: %d\n", socket_fd, actualCPU);
}

int main(int argc, char **argv){
	//Verificar Parametros Entrada
	if(argc <4){
		fprintf(stderr,"Syntax Error: Esperado: ./server MAX_PACKS NTHREADS DESTINATION_PORT\n");
		exit(1);
	}

	//Recuperar PID
	int pid = getpid();	
	if(mostrarInfo)	printf("El pid es %d\n", pid);

	//Recuperar total de paquetes a enviar
	MAX_PACKS = atoi(argv[1]);

	//Recuperar numero de Threads
	NTHREADS = atoi(argv[2]);
	pthread_t pids[NTHREADS];

	//Recuperar puerto destino
	DESTINATION_PORT = atoi(argv[3]);

	//Info
	int totalCPUs = sysconf(_SC_NPROCESSORS_ONLN);
	if(mostrarInfo) printf("Total de Procesadores disponibles: %d\n", totalCPUs);

	//Crear Socket
	int socket_fd;
	char ports[10];
	sprintf(ports, "%d", DESTINATION_PORT);
	socket_fd = udp_bind(ports);
	if(socket_fd < 0) {
		fprintf(stderr, "Error de bind al tomar el puerto\n");
		exit(1);
	}

	pthread_mutex_init(&lock, NULL);
	
		//Inicar FTRACE
		// Primero, Abrir Archivos para cosas de FTRACE
		printf("1.- Abrir archivos\n");
		printf("\t%s", trace_path);
		trace_fd = open(trace_path, O_WRONLY);
		if(trace_fd < 1){
			printf("\tError\n");
			exit(1);
		}else{
			printf("\tOK\n");
		}
		printf("\t%s", pid_trace_path);
		pid_trace_fd = open(pid_trace_path, O_WRONLY);
		if(pid_trace_fd < 1){
			printf("\tError\n");
			exit(1);
		}else{
			printf("\tOK\n");
		}
		printf("\t%s", marker_path);
		marker_fd = open(marker_path, O_WRONLY);
		if(marker_fd < 1){
			printf("\tError\n");
			exit(1);
		}else{
			printf("\tOK\n");
		}
		// Y activar Ftrace
		write(trace_fd, "1", 1);
		char tmp[12]={0x0};
		sprintf(tmp,"%d", pid);
		write(pid_trace_fd, tmp, sizeof(tmp));

	//Configurar Threads
    pthread_attr_t attr;
    cpu_set_t cpus;
    pthread_attr_init(&attr);

	//Lanzar Threads
	int i;
	for(i=0; i < NTHREADS; i++) {
		CPU_ZERO(&cpus);
		CPU_SET(i%totalCPUs, &cpus);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);

		if(distribuiteCPUs){
			pthread_create(&pids[i], &attr, llamadaHilo, socket_fd);
		}else{
			pthread_create(&pids[i], NULL, llamadaHilo, socket_fd);
		}
		
	}

	//Esperar Threads
	for(i=0; i < NTHREADS; i++) 
		pthread_join(pids[i], NULL);

		// Finalmente, apagar Ftrace
		write(marker_fd, "MITRACE: Todos los threads terminados\n", 38);
		write(trace_fd, "0", 1);
		// Y cierre de archivos
		close(trace_fd);
		close(pid_trace_fd);
		close(marker_fd);

	//Medir Fin
	gettimeofday(&dateFin, NULL);

	//Cerrar Socket
	close(socket_fd);

	segundos=(dateFin.tv_sec*1.0+dateFin.tv_usec/1000000.)-(dateInicio.tv_sec*1.0+dateInicio.tv_usec/1000000.);
	if(mostrarInfo){
		printf("Tiempo Total = %g\n", segundos);
		printf("QPS = %g\n", MAX_PACKS*1.0/segundos);
	}else{
		printf("%g, \n", segundos);
	}
	exit(0);
}