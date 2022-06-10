#include <iostream> // Para cout, cerr
#include <stdexcept> // Para declaraci�n de la clase runtime_error
#include <cstring> // Para declaraci�n de memset
using namespace std;
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<semaphore.h> // Para implementar sem�foros
#include<pthread.h>
#include<sched.h>
// Para funciones y estructuras de datos POSIX para comunicaci�n UDP
// ----------------- Macros --------------------
#define PUERTO_SERVIDOR 8000
// Puerto utilizado por este servidor para recibir y enviar datagramas
// bajo protocolo UDP
#define LONG_BUFFER 500
// N�mero de bytes en el buffer donde se recogen los datagramas en este
// programa
// --------------- Variables ------------------
uint8_t buffer[LONG_BUFFER];
// Buffer para manejar paquetes de bytes
struct sockaddr_in direccionServidor;
// Estructura para expresar la IP y el puerto utilizados por este servidor
struct sockaddr_in direccionCliente;
// Estructura para expresar la IP y el puerto utilizados por el �ltimo cliente
// que envi� un datagrama
socklen_t tamanoDireccionCliente;
// Tama�o de la estructura direccionCliente
int s; // Socket utilizado para la comunicaci�n en red

float Kp=0.45;
float Ki=0.35;
float Ts=0.5;
uint16_t Consigna = 100;
// Los valores de las constantes Kp y Ki se han determinado en Matlab a base de ensayo-error

sem_t semaforo;

float actuacionActual;
float lecturaActual;

void *hiloComunicacion(void *p){
	try {
		tamanoDireccionCliente = sizeof(direccionCliente);
		// Obtiene el tama�o de la estructura 'direccionCliente'
		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s == -1)
			throw runtime_error("Error en la creaci�n del socket");
		// Crea un socket para comunicaci�n bajo protocolo UDP para la recepci�n y
		// env�o de datagramas. Si hay alg�n error en la creaci�n
		// del socket, genera una excepci�n con un mensaje. Para ello se crea un
		// nuevo objeto de la clase runtime_error, pasando el mensaje a su constructor.
		// El mensaje se crea mediante objetos de la clase string y a partir del entero 'errno'
		// que contiene un valor diferente en funci�n del tipo del error.
		memset((char *) &direccionServidor, 0, sizeof(direccionServidor));
		// Pone a cero todos los bytes de esta estructura de datos, para inicializarla
		direccionServidor.sin_family = AF_INET;
		// Para indicar que es una comunicaci�n en red
		direccionServidor.sin_port = htons(PUERTO_SERVIDOR);
		// Para asignar un n�mero de puerto
		direccionServidor.sin_addr.s_addr = htonl(INADDR_ANY);
		// Para indicar qu� direcci�n IP utiliza esta aplicaci�n. Con INADDR_ANY esta aplicaci�n
		// utiliza todas las direcciones IP disponibles en todos los adaptadores de red en
		// funcionamiento en el sistema donde se ejecuta, de forma que un cliente puede
		// enviarle informaci�n a trav�s de todas las conexiones posibles.
		if (bind(s, (struct sockaddr*) &direccionServidor,
				sizeof(struct sockaddr)) == -1)
			throw runtime_error("Error asignando IP y puerto");
		// Asocia el direccionamiento indicado en la estructura direccionServidor al socket s.
		// Si hay alg�n problema, lanza una excepci�n con un mensaje

		while (1) { // Repite indefinidamente ...

			cout << "Esperando bloqueado la recepci�n de un datagrama ..."
					<< endl;
			int nBytesRecibidos = recvfrom(s, buffer, LONG_BUFFER, 0,
					(struct sockaddr *) &direccionCliente,
					&tamanoDireccionCliente);
			// Espera bloqueado la recepci�n de un datagrama. Par�metros:
			// - s: socket que indica a trav�s de qu� IP y puerto se va a recibir
			// - buffer: matriz de bytes donde se recibe el datagrama
			// - LONG_BUFFER_RECEPCION: tama�o disponible en el buffer, en bytes
			// - 0: en este par�metro se pueden indicar valores diferentes de 0 para
			// indicar diferentes opciones para la recepci�n. En este caso se indica
			// un 0 para que sea una recepci�n convencional.
			// - direccionCliente: en esa estructura esta funci�n guarda informaci�n sobre el
			// cliente que nos envi� el datagrama
			// - tamanoDireccionCliente: tama�o de la estructura direccionCliente
			if (nBytesRecibidos == -1)
				throw runtime_error("Error recibiendo datos");
			if (nBytesRecibidos != 4 + 4 + 4 + 2)
				throw runtime_error("Error en longitud de datagrama recibido");
			// Si hubo alg�n error en la recepci�n, lanza una excepci�n con un mensaje
			cout << "Se ha recibido un datagrama de " << nBytesRecibidos
					<< " bytes del cliente " << inet_ntoa(
					direccionCliente.sin_addr) << ":"
					<< ntohs(direccionCliente.sin_port) << endl;
			// Muestra la IP y puerto utilizados por el cliente

			sem_wait(&semaforo);

			Kp = *(float*) buffer;
			Ki = *(float*) (buffer + 4);
			Ts = *(float*) (buffer + 8);
			Consigna = *(uint16_t*) (buffer + 12);
			// Asigna a las constantes sus valores, seg�n la posici�n que ocupan en el paquete

			sem_post(&semaforo);


			cout << "Kp: " << Kp << endl;
			cout << "Ki: " << Ki << endl;
			cout << "Ts: " << Ts << endl;
			cout << "Consigna: " << Consigna << endl;
			// Copia los datos recibidos en las variables

			memcpy(buffer, &lecturaActual, 4);
			memcpy(buffer + 4, &actuacionActual, 4);
			// Visualiza en pantalla los datos recibidos

			int nBytesEnviados = sendto(s, buffer, 4 + 4, 0,
					(struct sockaddr *) &direccionCliente,
					tamanoDireccionCliente);
			// Env�a al cliente el paquete
			if (nBytesEnviados == -1)
				throw runtime_error("Error en la transmisi�n");
			if (nBytesEnviados != 4 + 4)
				throw runtime_error(
						"Error en la longitud del datagrama transmitido");
			// Lanza una excepci�n si hubo alg�n problema en la transmisi�n

		}
	} catch (exception & ex) {
		// Captura cualquier excepci�n producida en el bloque try{} y que gener� un objeto
		// de alguna clase derivada de la clase 'exception' (por ejemplo, de la clase
		// 'runtime_error')
		cerr << ex.what() << endl;
		// Visualiza en la salida est�ndar de error (por defecto, la consola) el texto
		// guardado en el objeto referenciado por 'ex'
	}
}

void *hiloControl(void *p) {

	float lecturaAnterior[10]={0,0,0,0,0,0,0,0,0,0};
	float errorActual=0;
	float errorAnterior=0;
	float actuacionAnterior=0;
	int i = 0;

		while(1) { // Repite indefinidamente ...

			delay((uint16_t)(Ts*1000));

			sem_wait(&semaforo);

						errorActual = Consigna - lecturaActual; // C�lculo del error
						actuacionActual = actuacionAnterior + ((Kp * (errorActual - errorAnterior)) + (Ki * Ts * errorActual)); // Control proporcional-integral

					    lecturaActual=(0.7575*lecturaAnterior[0])+(1.213*actuacionAnterior); // Ecuaci�n en diferencias discretizada
						// La planta se ha obtenido a partir de Matlab y Simulink, resultado de discretizar un sistema de primer orden sin retardo

					    lecturaAnterior[0]=lecturaActual; // Simulaci�n de un retardo
					    for (i=0; i>=10; i--){
						lecturaAnterior[i]=lecturaAnterior[i+1]; //
					    }
						errorAnterior = errorActual; // Actualizaci�n de variables
						actuacionAnterior = actuacionActual; // Actualizaci�n de variables


			sem_post(&semaforo);
		}
}

int main(void) {

	pthread_t hComunicacion; // Asigna ID al hilo
	pthread_t hControl; // Asigna ID al hilo

	sem_init(&semaforo, 0, 1); // Inicializa el sem�foro

	pthread_create(&hComunicacion, NULL, hiloComunicacion, NULL); // Crea el hilo encargado de la comunicaci�n
	pthread_create(&hControl, NULL, hiloControl, NULL); // Crea el hilo encargado del control de la planta


	pthread_join(hControl, NULL); // Espera a que finalice el hilo
	pthread_join(hComunicacion, NULL); // Espera a que finalice el hilo

	return EXIT_SUCCESS;
}
