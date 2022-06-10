#include <iostream> // Para cout, cerr
#include <stdexcept> // Para declaración de la clase runtime_error
#include <cstring> // Para declaración de memset
using namespace std;
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<semaphore.h> // Para implementar semáforos
#include<pthread.h>
#include<sched.h>
// Para funciones y estructuras de datos POSIX para comunicación UDP
// ----------------- Macros --------------------
#define PUERTO_SERVIDOR 8000
// Puerto utilizado por este servidor para recibir y enviar datagramas
// bajo protocolo UDP
#define LONG_BUFFER 500
// Número de bytes en el buffer donde se recogen los datagramas en este
// programa
// --------------- Variables ------------------
uint8_t buffer[LONG_BUFFER];
// Buffer para manejar paquetes de bytes
struct sockaddr_in direccionServidor;
// Estructura para expresar la IP y el puerto utilizados por este servidor
struct sockaddr_in direccionCliente;
// Estructura para expresar la IP y el puerto utilizados por el último cliente
// que envió un datagrama
socklen_t tamanoDireccionCliente;
// Tamaño de la estructura direccionCliente
int s; // Socket utilizado para la comunicación en red

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
		// Obtiene el tamaño de la estructura 'direccionCliente'
		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s == -1)
			throw runtime_error("Error en la creación del socket");
		// Crea un socket para comunicación bajo protocolo UDP para la recepción y
		// envío de datagramas. Si hay algún error en la creación
		// del socket, genera una excepción con un mensaje. Para ello se crea un
		// nuevo objeto de la clase runtime_error, pasando el mensaje a su constructor.
		// El mensaje se crea mediante objetos de la clase string y a partir del entero 'errno'
		// que contiene un valor diferente en función del tipo del error.
		memset((char *) &direccionServidor, 0, sizeof(direccionServidor));
		// Pone a cero todos los bytes de esta estructura de datos, para inicializarla
		direccionServidor.sin_family = AF_INET;
		// Para indicar que es una comunicación en red
		direccionServidor.sin_port = htons(PUERTO_SERVIDOR);
		// Para asignar un número de puerto
		direccionServidor.sin_addr.s_addr = htonl(INADDR_ANY);
		// Para indicar qué dirección IP utiliza esta aplicación. Con INADDR_ANY esta aplicación
		// utiliza todas las direcciones IP disponibles en todos los adaptadores de red en
		// funcionamiento en el sistema donde se ejecuta, de forma que un cliente puede
		// enviarle información a través de todas las conexiones posibles.
		if (bind(s, (struct sockaddr*) &direccionServidor,
				sizeof(struct sockaddr)) == -1)
			throw runtime_error("Error asignando IP y puerto");
		// Asocia el direccionamiento indicado en la estructura direccionServidor al socket s.
		// Si hay algún problema, lanza una excepción con un mensaje

		while (1) { // Repite indefinidamente ...

			cout << "Esperando bloqueado la recepción de un datagrama ..."
					<< endl;
			int nBytesRecibidos = recvfrom(s, buffer, LONG_BUFFER, 0,
					(struct sockaddr *) &direccionCliente,
					&tamanoDireccionCliente);
			// Espera bloqueado la recepción de un datagrama. Parámetros:
			// - s: socket que indica a través de qué IP y puerto se va a recibir
			// - buffer: matriz de bytes donde se recibe el datagrama
			// - LONG_BUFFER_RECEPCION: tamaño disponible en el buffer, en bytes
			// - 0: en este parámetro se pueden indicar valores diferentes de 0 para
			// indicar diferentes opciones para la recepción. En este caso se indica
			// un 0 para que sea una recepción convencional.
			// - direccionCliente: en esa estructura esta función guarda información sobre el
			// cliente que nos envió el datagrama
			// - tamanoDireccionCliente: tamaño de la estructura direccionCliente
			if (nBytesRecibidos == -1)
				throw runtime_error("Error recibiendo datos");
			if (nBytesRecibidos != 4 + 4 + 4 + 2)
				throw runtime_error("Error en longitud de datagrama recibido");
			// Si hubo algún error en la recepción, lanza una excepción con un mensaje
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
			// Asigna a las constantes sus valores, según la posición que ocupan en el paquete

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
			// Envía al cliente el paquete
			if (nBytesEnviados == -1)
				throw runtime_error("Error en la transmisión");
			if (nBytesEnviados != 4 + 4)
				throw runtime_error(
						"Error en la longitud del datagrama transmitido");
			// Lanza una excepción si hubo algún problema en la transmisión

		}
	} catch (exception & ex) {
		// Captura cualquier excepción producida en el bloque try{} y que generó un objeto
		// de alguna clase derivada de la clase 'exception' (por ejemplo, de la clase
		// 'runtime_error')
		cerr << ex.what() << endl;
		// Visualiza en la salida estándar de error (por defecto, la consola) el texto
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

						errorActual = Consigna - lecturaActual; // Cálculo del error
						actuacionActual = actuacionAnterior + ((Kp * (errorActual - errorAnterior)) + (Ki * Ts * errorActual)); // Control proporcional-integral

					    lecturaActual=(0.7575*lecturaAnterior[0])+(1.213*actuacionAnterior); // Ecuación en diferencias discretizada
						// La planta se ha obtenido a partir de Matlab y Simulink, resultado de discretizar un sistema de primer orden sin retardo

					    lecturaAnterior[0]=lecturaActual; // Simulación de un retardo
					    for (i=0; i>=10; i--){
						lecturaAnterior[i]=lecturaAnterior[i+1]; //
					    }
						errorAnterior = errorActual; // Actualización de variables
						actuacionAnterior = actuacionActual; // Actualización de variables


			sem_post(&semaforo);
		}
}

int main(void) {

	pthread_t hComunicacion; // Asigna ID al hilo
	pthread_t hControl; // Asigna ID al hilo

	sem_init(&semaforo, 0, 1); // Inicializa el semáforo

	pthread_create(&hComunicacion, NULL, hiloComunicacion, NULL); // Crea el hilo encargado de la comunicación
	pthread_create(&hControl, NULL, hiloControl, NULL); // Crea el hilo encargado del control de la planta


	pthread_join(hControl, NULL); // Espera a que finalice el hilo
	pthread_join(hComunicacion, NULL); // Espera a que finalice el hilo

	return EXIT_SUCCESS;
}
