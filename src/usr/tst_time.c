#include <usr/libsodium.h>

int iFnImprimirNumero( const char *cncpBuffer, int iNumero );
unsigned long liFnProcesoInverso(int iFecha[]);


int main(){

	// Declaracion de variables
	// Utilizadas para las pruebas del SetTime	
	long delay=500000;
	int i, j, k, l, iN;
	int iFecha[6];
	unsigned long lisegundosTranscurridos;
	time_t *ll;
  	int iSecuenciaAnioBisiesto[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int iSecuenciaAnioNoBisiesto[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};	


	// Utilizadas por el settimeofday
	unsigned long liValor;

	// Utilizadas por el gettimeofday
	timezone hh;
	timeval time;

	// Utilizada por el adjtimex
	timex timexVar;

	// Utilizada por el times
	tms timesss;
	clock_t pulsosDesdeInicioSistema;

	// Utilizada por el nanosleep
	timespec req, rem;

	// Utilizada por los timer
	itimerval timer, oTimer, getTimer;

	write( 0, "\n", 0 );
	//Probamos el funcionamiento de la stime
	for(i=2006; i<2007; i++)
	{
		// Prueba ultimo dia del mes
		for(j=0; j<1; j++)
		{

			iFecha[0] = i;
			iFecha[1] = j+1;

 	  		if((i % 4 == 0) && ((i % 100 != 0) || (i %400 == 0)) == 0)
			{
				iFecha[2] = iSecuenciaAnioBisiesto[j];
			}
			else
			{
				iFecha[2] = iSecuenciaAnioNoBisiesto[j];
			}
		iFecha[3] = 0;
		iFecha[4] = 0;
		iFecha[5] = 0;

		lisegundosTranscurridos =  liFnProcesoInverso(iFecha);

		*ll = (time_t) lisegundosTranscurridos;
		iFnImprimirNumero("Cantidad de segundos transcurridos: ",  lisegundosTranscurridos);

		write( 0, "\nDia a setear en el CMOS: ", 0 );
		for (k=0; k<6; k++)
		{
			write( 0, "   ", 0 );
			systest(iFecha[k]);
		}
		
  		stime(ll);
		for(l=0;l<delay;l++);			


		// Prueba primer dia del mes
   		iFecha[2] = 1;
		lisegundosTranscurridos =  liFnProcesoInverso(iFecha);
		*ll = (time_t) lisegundosTranscurridos;

		iFnImprimirNumero("\n\nCantidad de segundos transcurridos: ",  lisegundosTranscurridos);

		write( 0, "\nDia a setear en el CMOS: ", 0 );
		for (k=0; k<6; k++)
		{
			write( 0, "   ", 0 );
			systest(iFecha[k]);
		}	

  		stime(ll);

		for(l=0;l<delay;l++);			
		write( 0, "\n\n", 0 );

	      }
	}
	// Probamos el funcionamiento de settimeofDay
	// pondremos el mayor valor que puede recuperar el gettimeofday
	// 2^32 - 1
	liValor = 2147483647;
	time.tv_sec = liValor/1000;
	time.tv_usec = liValor - time.tv_sec*1000;
	hh.tz_minuteswest = 0;
	hh.tz_dsttime = 0;
	
	settimeofday(&time, &hh);

	// Probamos el funcionamiento de gettimeofDay
	write( 0, "Datos obtenidos por el gettimeofday", 0 );
	gettimeofday(&time, &hh);
	iFnImprimirNumero("Segundos transcurridos = ", time.tv_sec);	
	iFnImprimirNumero("Milisegundos transcurridos = ", time.tv_usec);	
	for(l=0;l<delay;l++);			


	// Probamos el adjTimex
	timexVar.modes = ADJ_TICK;
	timexVar.tick = 50;
	
	write( 0, "\n\nCambiamos la frecuencia del timer", 0 );
	adjtimex(&timexVar);
	iFnImprimirNumero("timexVar.time.tv_sec = ", timexVar.time.tv_sec);
	iFnImprimirNumero("timexVar.time.tv_usec = ", timexVar.time.tv_usec);
	iFnImprimirNumero("timexVar.tick = ", timexVar.tick);
	for(l=0;l<delay;l++);

	// Probamos el times
	write( 0, "\n\nTomamos los tiempos del proceso", 0 );
	pulsosDesdeInicioSistema = times(&timesss);
	iFnImprimirNumero("Pulsos de inicio =  ", pulsosDesdeInicioSistema);
	iFnImprimirNumero("Tiempo de usuario = ", timesss.tms_utime);
	iFnImprimirNumero("Tiempo de sistema = ", timesss.tms_stime);
	iFnImprimirNumero("Tiempo de usuario del hijo = ", timesss.tms_cutime);
	iFnImprimirNumero("Tiempo de sistema del hijo = ", timesss.tms_cstime);
	for(l=0;l<delay;l++);

	// Probamos el timer de tiempo real
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = 5;
	timer.it_value.tv_usec = 2;
	
	settimer(ITIMER_REAL, &timer, &oTimer);

	write( 0, "\n\nVemos los valores del timer", 0 );
	iFnImprimirNumero("Segundos del timer anterior = ", oTimer.it_value.tv_sec);
	iFnImprimirNumero("Milisegundos del timer anterior = ", oTimer.it_value.tv_usec);
	iFnImprimirNumero("Segundos del timer = ", timer.it_value.tv_sec);
	iFnImprimirNumero("Milisegundos del timer = ", timer.it_value.tv_usec);
	 // Generamos un delay
	for(l=0;l<100;l++);

	// Probamos el gettimer
	write( 0, "\n\nObtenemos los valores del timer creado antes", 0 );
	gettimer(ITIMER_REAL, &getTimer);
	iFnImprimirNumero("Segundos restantes = ", getTimer.it_value.tv_sec);
	iFnImprimirNumero("Milisegundos restantres = ", getTimer.it_value.tv_usec);
	
	write( 0, "\n\nMostramos el funcionamiento del nanosleep", 0 );
	// Probamos el nanosleep
	req.tv_sec = 15;
	req.tv_nsec = 13;
	rem.tv_sec = 0;
	rem.tv_nsec = 0;
		
	for (iN=0; iN<1; iN++){
		nanosleep(&req, &rem);
		iFnImprimirNumero("Despues i = ", iN);
	}

	iFnImprimirNumero("rem.tv_sec = ", rem.tv_sec);
	iFnImprimirNumero("rem.tv_nsec = ", rem.tv_nsec);
	
	return 0;
}

// Funcion que recibe una fecha en formato de vector (año-mes-dia-hora-minuto-segundo)
// y devuelve la cantidad de segundos que trascurrieron desde la fecha de inicio del sistema (definido en definiciones.h)
unsigned long liFnProcesoInverso(int iFecha[]){
	int iAnioActual = ANIO_INICIO;
	int iSecuenciaAnioBisiesto[13] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};
	int iSecuenciaAnioNoBisiesto[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
	unsigned long liSegundosAcumulados = 0;
// Las horas, minutos y segundos salen directo
	liSegundosAcumulados += iFecha[5] + iFecha[4]*60 + (iFecha[3] * 3600);

// Los dias transcurridos son 1 menos que el de la fecha actual
	liSegundosAcumulados += (iFecha[2] - 1) * 86400;


// Los meses transcurridos dependen de si el año es bisiesto
	if((iFecha[0] % 4 == 0) && ((iFecha[0] % 100 != 0) || (iFecha[0] %400 == 0)) == 1) {
		liSegundosAcumulados += iSecuenciaAnioBisiesto[iFecha[1]-1] * 86400;
	}else{
		liSegundosAcumulados += iSecuenciaAnioNoBisiesto[(iFecha[1]-1)] * 86400;
	}

// Los años van a depender de la cantidad de años bisiestos que pasaron
	for(; iFecha[0] > iAnioActual; iAnioActual++){
		if ((iAnioActual % 4) == 0){
			liSegundosAcumulados += 366 * 86400;
		}else{
			liSegundosAcumulados += 365 * 86400;
		}
	}
return liSegundosAcumulados;
}

int iFnImprimirNumero( const char *cncpBuffer, int iNumero ){
	write( 0, cncpBuffer, 0 );
	systest( iNumero );
	write( 0, "\n", 0 );
	return 0;
}

int iFnImprimir( const char *cncpBuffer){
	write( 0, cncpBuffer, 0 );
	write( 0, "\n", 0 );
	return 0;
}
