#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define N_PARADAS     5 // número de paradas de la ruta
#define EN_RUTA       0 // autobús en ruta
#define EN_PARADA     1 // autobús en la parada
#define MAX_PASAJEROS 3 // capacidad del autobús
#define PASAJEROS     5 // numero de usuarios

// estado inicial
int estado = EN_RUTA;
int parada_actual = 0; // parada en la que se encuentra el autobus
int n_ocupantes   = 0; // ocupantes que tiene el autobús

// personas que desean subir en cada parada
int esperando_subir[N_PARADAS]; //= {0,0,...0};
// personas que desean bajar en cada parada
int esperando_bajar[N_PARADAS]; //= {0,0,...0};

// Otras definiciones globales (comunicación y sincronización)
pthread_t autobus; // hilo del autobus
pthread_t pasajeros[PASAJEROS]; // hilos de los usuarios

pthread_mutex_t mtx; // mutex de bajar en cada parada

pthread_cond_t bajen;
pthread_cond_t suban;
pthread_cond_t pasajeroEnAutobus;
pthread_cond_t pasajeroEnParada;


// Ajustar el estado y bloquear al autobús hasta que no haya pasajeros que
// quieran bajar y/o subir la parada actual. Después se pone en marcha.
void Autobus_En_Parada(){
	// Cogemos los mutex para notificar a los pasajeros de la llegada.
	pthread_mutex_lock(&mtx);

	estado = EN_PARADA;
	printf("[STOP] Autobus en parada %d\n",parada_actual);
	if (esperando_subir[parada_actual] > 0)
		printf("   [+] %d pasajeros esperan a subir\n",esperando_subir[parada_actual]);
	if (esperando_bajar[parada_actual] > 0)
		printf("   [-] %d pasajeros esperan a bajar\n",esperando_bajar[parada_actual]);
	
	// El orden de bajar-subir es intencionado con el fin de evitar
	// que las personas que hayan bajado en esa parada no vuelvan a subir
	// en el mismo ciclo.

	// Aviso a los pasajeros para que bajen.
	pthread_cond_broadcast(&bajen);
	// Espera para que los pasajeron bajen
	// (En los wait se sueltan los mutex)
	while (esperando_bajar[parada_actual] > 0){
		pthread_cond_wait(&pasajeroEnParada,&mtx);
	}

	// Aviso a los pasajeros para que suban.
	pthread_cond_broadcast(&suban);
	// Espera para que los pasajeron suban.
	// (En los wait se sueltan los mutex)
	while (esperando_subir[parada_actual] > 0 && n_ocupantes < MAX_PASAJEROS){
		pthread_cond_wait(&pasajeroEnAutobus,&mtx);
	}

	estado = EN_RUTA;
	printf("[ROAD] Autobus en ruta\n");

	printf("   [~] %d pasajeros en el autobus\n",n_ocupantes);

	// Soltamos los mutex
	pthread_mutex_unlock(&mtx);
}

// Establecer un retardo que simule el trayecto y actualizar numero de parada.
void Conducir_Hasta_Siguiente_Parada(){
	sleep(1);
	// Recorrido circular.
	pthread_mutex_lock(&mtx);
	parada_actual = (parada_actual+1)%N_PARADAS;
	pthread_mutex_unlock(&mtx);
}

void*thread_autobus(void*args) {
	while (1) {
		// esperar a que los viajeros suban y bajen
		Autobus_En_Parada();
		// conducir hasta siguiente parada
		Conducir_Hasta_Siguiente_Parada();
	}
}


// El usuario indicará que quiere subir en la parada ’origen’,
// esperará a que el autobús se pare en dicha parada y subirá.
// El id_usuario puede utilizarse para proporcionar información de depuración.
void Subir_Autobus(int id_usuario, int origen){
	// Cogemos mutex para modificar variables compartidas.
	pthread_mutex_lock(&mtx);
	
	esperando_subir[origen]++;
	printf("       Pasajero %d esperando subir en parada %d\n",id_usuario,origen);
	while (estado != EN_PARADA || parada_actual != origen || n_ocupantes >= MAX_PASAJEROS){
		pthread_cond_wait(&suban,&mtx);
	}
	n_ocupantes++; // solo se modifica en la parada actual
	esperando_subir[origen]--; 
	printf("       Pasajero %d sube al autobus\n",id_usuario);

	pthread_cond_signal(&pasajeroEnAutobus); // notificamos al autobus que estamos dentro

	// Soltamos mutex.
	pthread_mutex_unlock(&mtx);
}

// El usuario indicará que quiere bajar en la parada ’destino’,
// esperara a que el autobús se pare en dicha parada y bajará.
// El id_usuario puede utilizarse para proporcionar información de depuración.
void Bajar_Autobus(int id_usuario, int destino){
	// Cogemos mutex para modoficar variables compartidas.
	pthread_mutex_lock(&mtx);

	esperando_bajar[destino]++;
	printf("       Pasajero %d esperando bajar en parada %d\n",id_usuario,destino);
	while (estado != EN_PARADA || parada_actual != destino){
		pthread_cond_wait(&bajen,&mtx);
	}
	n_ocupantes--; // solo se modifica en la parada actual
	esperando_bajar[destino]--;
	printf("       Pasajero %d baja del autobus\n",id_usuario);
	
	pthread_cond_signal(&pasajeroEnParada); // notificamos al autobus que estamos fuera

	// Soltamos mutex.
	pthread_mutex_unlock(&mtx);
}

void Usuario(int id_usuario, int origen, int destino) {
	// Esperar a que el autobus esté en parada origen para subir
	Subir_Autobus(id_usuario, origen);
	// Bajarme en estación destino
	Bajar_Autobus(id_usuario, destino);
}

void*thread_usuario(void * arg) {
	int a, b, id_usuario = (int) arg;
	// obtener el id del usuario
	while (1) {
		a = rand() % N_PARADAS;
		do {
			b = rand() % N_PARADAS;
		} while (a == b);
		Usuario(id_usuario,a,b);
	}
}


// Definición de variables locales a main
// Opcional: obtener de los argumentos del programa la capacidad del
// autobus, el numero de usuarios y el numero de paradas
int main(int argc, char*argv[]) {
	int i;

	// Inicializamos el mutex
	pthread_mutex_init(&mtx,NULL);

	// Inicializamos las variables condicion
	pthread_cond_init(&bajen,NULL);
	pthread_cond_init(&suban,NULL);
	pthread_cond_init(&pasajeroEnAutobus,NULL);
	pthread_cond_init(&pasajeroEnParada,NULL);

	// Crear el thread autobus
	pthread_create(&autobus,NULL,thread_autobus,NULL);
	for (i = 0; i < PASAJEROS; ++i){
		// Crear thread para el usuario i
		pthread_create(&pasajeros[i],NULL,thread_usuario,(void *) i);
	}

	// Esperar terminación del autobus
	pthread_join(autobus,NULL);
	for (i = 0; i < PASAJEROS; ++i){
		// Esperar terminación para el usuario i
		pthread_join(pasajeros[i],NULL);
	}

	// Destruimos las variables condicion
	pthread_cond_destroy(&bajen);
	pthread_cond_destroy(&suban);
	pthread_cond_destroy(&pasajeroEnAutobus);
	pthread_cond_destroy(&pasajeroEnParada);

	// Destruimos los mutex
	pthread_mutex_destroy(&mtx);

	return 0;
}