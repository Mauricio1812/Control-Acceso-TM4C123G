/*********************************************************************/
/*********************************************************************/
/**            PONTIFICIA UNIVERSIDAD CATOLICA DEL PERÚ             **/
/**               FACULTAD DE CIENCIAS E INGENIERIA                 **/
/**                     SISTEMAS DIGITALES                          **/
/*********************************************************************/
/**      Microcontrolador:   TM4C123GH6PM                           **/
/**      EvalBoard:          Tiva C Series TM4C123G LaunchPad       **/
/**      Autores:            Mauricio Salazar y Dany Medina       	**/
/**      Fecha:              Noviembre 2021                         **/
/*********************************************************************/
/**      Enunciado:                                       			**/
/**  Control de acceso con memoria de 10 usuarios. Permite registrar **/
/**  nuevos con una clave admin y cambiar dicha clave de administrador**/
/***********************************************************************/

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "Nokia5110.h"
	
/*Funcion de configuracion del puerto F*/
void config_portF(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;            // Activar el Reloj de Puerto F
    while( (SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5)==0) { } // Se espera a que se active el reloj
		
		GPIO_PORTF_LOCK_R = 0x4C4F434B;											// Activar configuracion 
		GPIO_PORTF_CR_R |= 0x01;														// de PF0
		
		GPIO_PORTF_DIR_R |= 0x0E;                           // Configura pines PF1-3 como salidas
		GPIO_PORTF_DIR_R &= ~(0x11);                        // Configura pines PF0+PF4 como entradas. 
		GPIO_PORTF_PUR_R = 0x11;														//Activar pull-up para PF0 y PF4		
		GPIO_PORTF_AFSEL_R &= ~0x11;													// Desactivar funciones alternas    
		GPIO_PORTF_DEN_R |= 0x1F;                           // Activar pines PF0-PF4 como digital	
		
		GPIO_PORTF_DATA_R &= ~(0x0E); 											//Apaga todos los LEDS  
}

//********Parpadeo*****************
// Parpadeo de led de color seleccionado 
// ENTRADAS: color
// SALIDA: ninguna
void Parpadeo(uint32_t color){
	uint32_t i;
	
  GPIO_PORTF_DATA_R |= color;          // Prender Led [color]
  for(i=0;i<=3200000;i++);              
	GPIO_PORTF_DATA_R &= ~(color);				// Apagar Led [color]	
  for(i=0;i<=3200000;i++); 	
}

/*Funcion de configuracion del UART2*/
void config_uart2(void){
	SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R2; 						//Habilitamos UART2
	while((SYSCTL_RCGCUART_R & SYSCTL_RCGCUART_R2)==0){}; 	// Esperamos a que se active UART2
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3; 						//Activamos el puerto D
	while((SYSCTL_PRGPIO_R & SYSCTL_RCGCGPIO_R3)==0){};		//Esperamos a que se active el reloj de D
	
	// Configuracion para 9600 bps, 1 parada, sin paridad (Default de HC-05)
  UART2_CTL_R  = 0x0;       												//Se desactiva el UART
  UART2_IBRD_R = (UART2_IBRD_R & (~0xFFFF)) + 104;	//Parte entera
	UART2_FBRD_R = (UART2_FBRD_R & (~0x3F)) + 11; 		// Parte decimal
	UART2_LCRH_R = (UART2_LCRH_R & (~0xFF)) | 0x70; 	// 8 bits de datos, sin paridad, 1 bit de parada, FIFOs habilitados
	UART2_CTL_R &= ~(0x20); 													// HSE=0,
	UART2_CTL_R |= 0x301; 														// Para habilitar UARTEN=1, RXE=1, TX=1
	
	//Configuracion portD
	GPIO_PORTD_LOCK_R = 0x4C4F434B;				// Activar configuracion 
	GPIO_PORTD_CR_R |= 0xF0;		
	
	GPIO_PORTD_DIR_R |= 0x80; 						//PD7 como salida
	GPIO_PORTD_DIR_R &= ~(0x40); 					//PD6 como entrada
	GPIO_PORTD_DEN_R |= 0xC0; 						// Funciones digitales en PD6 y PD7	
	GPIO_PORTD_AMSEL_R &= ~(0xC0);				// Desactivamos modo analogico en PD6 y PD7	
	GPIO_PORTD_AFSEL_R |= 0xC0;						// Activamos funciones alternas en PD6 y PD7		
  GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R&0x00FFFFFF)|0x11000000;	// Conecta UART2 a PD6 y PD7		
}

/*Funcion para la transmision de un caracter*/
void txcar_uart2(uint32_t car){
 while ((UART2_FR_R & UART_FR_TXFF)!=0); //Espera que este disponible para tx
 UART2_DR_R = car;
}

/*Funcion para la recepcion de un caracter de 8 bits*/
uint8_t rxcar_uart2(void){
	uint8_t temp;
	while ((UART2_FR_R & UART_FR_RXFE)!=0){}; // Se espera que llegue un dato
	temp= UART2_DR_R&0xFF; // Se toman solo 8 bits
	
		return temp;
}

/*Funcion para el envio de una cadena*/ 
void txmens_uart2( uint8_t mens[] ){
 uint8_t letra;
 uint8_t i = 0;
 letra = mens[ i++ ];
 while( letra != '\0' ){
		//Se envian todos los caracteres hasta el fin de cadena
	 txcar_uart2( letra );
	 letra = mens[ i++ ];
 }
}

//********char_validos*****************
// Verificacion de que el input sea solo letras o numeros
// ENTRADA: ingreso[], longPalabra
// SALIDA: Condicion (0 o 1)
uint8_t char_validos(uint8_t ingreso[], uint8_t longPalabra){
	uint8_t invalidos,condicion,char_invalido,i;
	invalidos=0;
	
	for( i=0; i < longPalabra; i++ ){									//Evalua cada caracter del input
		char_invalido=1; //Caracter invalido 
		if(('0'<=ingreso[i])&&('z'>=ingreso[i])){
			char_invalido=0; //Caracter valido si esta entre '0' y 'z' (ASCII)
			if((('9'<ingreso[i])&&('A'>ingreso[i]))||(('Z'<ingreso[i])&&('a'>ingreso[i]))){
				char_invalido=1; //Caracter invalido si es algun simbolo no letra/numero entre '0' y 'z' (ASCII)
			}
		}
		invalidos=invalidos+char_invalido; //Se suma la cantidad de caracteres invalidos
	}	
				
	//Definiendo si se cumple la condicion
	if(invalidos){		//Si hay 1+ caracteres invalidos, no se cumple
		condicion=0; 	
	}else{ 					//Si hay 0 caracteres invalidos, se cumple
		condicion=1;}	
	
	return condicion;
}

//********input*****************
// Ingreso de cadena de caracteres hasta presionar Enter contando su longitud.
// ENTRADAS: *ingreso (direccion de palabra a ingresar), *longPalabra (registro de longitud de palabra ingresada)
// ENTRADAS: visible (Mostrar caracteres o *), longMAX (Longitud maxima del arreglo a ingresar)
// SALIDA: ninguna
void input(uint8_t *ingreso, uint8_t *longPalabra, uint8_t visible, uint8_t longMAX){
	uint8_t confirmar,i;
	uint32_t retardo;
	uint8_t oculta[] = "*";
	uint8_t longitud=0;
	
	for( i=0; (confirmar!=0x0D); i++ ){		
		confirmar=rxcar_uart2();		
		if(i<longMAX){ingreso[i] = confirmar;}
		if(confirmar!=0x0D){ //Si no se presiona ENTER
			longitud++; //Aumentando longitud
			if(visible){
				txcar_uart2(confirmar);
				Nokia5110_OutChar(confirmar);
				}
			else{
				txmens_uart2(oculta);
			  Nokia5110_OutString("*");}			
		}
	}
	*longPalabra=longitud;		
	
	//Borrando el resto del arreglo donde se almacena
	for( i=longitud; i<longMAX; i++ ){	
		ingreso[i]=0;
	}
	//Retardo para mostrar en LCD
	for(retardo=0;retardo<=3200000;retardo++);	
}

//********nueva*****************
// Registro de nuevo usuario o clave
// ENTRADAS: *nuevo (direccion de nuevo usuario/clave a registrar)
// ENTRADAS: tipo (1: Usuario, 0: Clave)
// SALIDA: ninguna
void nueva(uint8_t *nuevo, uint8_t tipo){
	uint8_t longNuevo,valida,longMAX;
	uint8_t mUsuario[] = "\n\rUsuario: ";
	uint8_t mClave[] = "\n\rNueva clave: ";
	uint8_t mInvalida[] = "\n\rLongitud o caracteres invalidos";
	
	do{
		Nokia5110_Clear();
		if(tipo){
			//Pantalla LCD
			Nokia5110_OutString("Usuario:");
			//UART
			txmens_uart2(mUsuario);	
			longMAX=20;
		}else{
			//Pantalla LCD
			Nokia5110_OutString("Clave:");
			//UART
			txmens_uart2(mClave);	
			longMAX=5;			
		}
		Nokia5110_SetCursor(0,1);
		
		//Ingresando
		input(&nuevo[0],&longNuevo,tipo,longMAX);
		//Validando	
		valida=(longNuevo>1)&&(longNuevo<(longMAX+1))&&char_validos(nuevo,longNuevo);
		
		if(!valida){ //Caso invalido
			txmens_uart2(mInvalida);
			Nokia5110_Clear();
			if(tipo){
				Nokia5110_OutString("Usuario");
				Nokia5110_SetCursor(0,1);
				Nokia5110_OutString("Invalido");				
			}else{
				Nokia5110_OutString("Clave");
				Nokia5110_SetCursor(0,1);
				Nokia5110_OutString("Invalida");				
			}
			Parpadeo(0x02);}		
	}while(!valida);	
}

//********input_maestra*****************
// Ingreso y validacion de clave maestra
// ENTRADAS: maestra[] (Arreglo de clave maestra registrada)
// SALIDA: valida (1 o 0)
uint8_t input_maestra(uint8_t maestra[]){
	uint8_t mMaestra[] = "Ingrese clave maestra: \n\r";	
	uint8_t valida,longPalabra,k;
	uint8_t ingreso[5];
	
	//UART
	txmens_uart2(mMaestra); 
	//LCD
	Nokia5110_Clear();
	Nokia5110_OutString("Ingrese");
	Nokia5110_SetCursor(0,1);
	Nokia5110_OutString("clave");	
	Nokia5110_SetCursor(0,2);		
	Nokia5110_OutString("maestra:");
	Nokia5110_SetCursor(0,3);				
	//Ingresar clave maestra
	input(&ingreso[0],&longPalabra,0,5);
			
	//Resetear verificacion
	valida=1;
	//Verificar clave maestra
	if(longPalabra<6){ //Si tiene misma longitud de clave maestra
		for( k=0; k < 5; k++ ){		
			if(ingreso[k]!=maestra[k]){
				valida=0;}
		}
	}else{valida=0;}
	return valida;	
}


/**********************************/
/********PROGRAMA PRINCIPAL********/
/**********************************/
int main( void ){
/**********************************************/
/**ETAPA0: Inicializacion puertos y variables**/	
/**********************************************/
	
 //Mensajes a utilizar
	uint8_t mAdmin[] = "Clave de administrador ADMIN:\n\r 1.Cambiar\n\r 2.Mantener\n\r";
	uint8_t mNuevoAdmin[] = "\n\rNueva clave de adminsitrador: ";
	uint8_t mInicio[] = "\n\rMENU INICIO\n\r 1.Ingresar\n\r 2.Registrar usuario\n\r 3.Borrar usuario\n\r";
	uint8_t mClave[] = "Ingrese clave de ingreso: ";
	uint8_t mFin1[] = "\n\rSea bienvenido usuario ";
	uint8_t mFin2[] = "\n\rClave incorrecta\n\r";
	uint8_t mExistente[] = "\n\rCredencial ya existente\n\r";
	uint8_t mLleno[] = "\n\rMemoria llena, borre clave para registrar nuevos usuarios\n\r";
	uint8_t mUsuarioS[]="\n\rIngrese Usuario: ";
	uint8_t mUsuarioF[]="\n\rUsuario no valido\n\r";
	uint8_t mUsuarioR[]="\n\rSe ha registrado al usuario ";
	uint8_t mUsuarioB[]="\n\rBorrando credenciales de usuario ";
	
	//Verificacion + Claves
	uint8_t verificar[11];
	uint8_t claves[11][5],usuarios[11][20]; //usuario y clave 11
 
	// Clave maestra
	uint8_t maestra[5]="ADMIN";
	// Arreglo para guardar intento digitado
	uint8_t ingreso[20];
	
	//Variables a utilizar
	uint8_t usuario, i,k,ind,longPalabra,longUsuario,color;
	uint8_t confirmar,opcion,valida;
	//uint32_t retardo;
	
	//Inicializacion portF + LCD	+UART
  Nokia5110_Init();
  Nokia5110_Clear();	
	config_portF();			
	config_uart2();  
	
/**********************************************/
/****ETAPA1: Cambiar clave de administrador****/	
/**********************************************/
	//Definir clave de administrador
	//UART
	txmens_uart2(mAdmin);	
	//LCD
	Nokia5110_OutString("Clave de");
	Nokia5110_SetCursor(0,1);
	Nokia5110_OutString("admin:");
	Nokia5110_SetCursor(0,2);
	Nokia5110_OutString("ADMIN");
	Nokia5110_SetCursor(0,3);
	Nokia5110_OutString("1.Cambiar");
	Nokia5110_SetCursor(0,4);
	Nokia5110_OutString("2.Mantener");
	
	//MENU SELECCION ADMIN
	confirmar=0;
	while(confirmar!=0x0D){
		opcion=0;
		while((opcion!='1')&&(opcion!='2')){
			opcion=rxcar_uart2();
		}
		confirmar=rxcar_uart2();
	}
		
	if(opcion=='1'){ //1. Cambiar clave de admin
		nueva(&maestra[0],0);
		//UART
		txmens_uart2(mNuevoAdmin);
		txmens_uart2(maestra);
		//LCD
		Nokia5110_Clear();			
		Nokia5110_OutString("Nueva clave");
		Nokia5110_SetCursor(0,1);
		Nokia5110_OutString("de admin:");
		Nokia5110_SetCursor(0,2);
		Nokia5110_OutString((char*)maestra);
		//Confirmar cambio con parpadeo VERDE
		Parpadeo(0x08);		
	}	
	
	//Inicializando claves
	for( k=0; k < 11; k++ ){	
		for( i=0; i < 5; i++ ){
			claves[k][i]=maestra[i];
		}
	}	
	//Inicializando usuario administrador
	usuarios[10][0]='A';
	usuarios[10][1]='d';
	usuarios[10][2]='m';
	usuarios[10][3]='i';
	usuarios[10][4]='n';
		
	while(1){
/**********************************************/
/*******ETAPA2: Seleccion Menu de inicio*******/	
/**********************************************/	
		txmens_uart2(mInicio);
		Nokia5110_Clear();
    Nokia5110_OutString("Opciones:");
		Nokia5110_SetCursor(0,1);
		Nokia5110_OutString("1.Ingresar ");	
		Nokia5110_SetCursor(0,2);		
		Nokia5110_OutString("2.Nuevo");
		Nokia5110_SetCursor(0,3);	
		Nokia5110_OutString("usuario");
		Nokia5110_SetCursor(0,4);	
		Nokia5110_OutString("3.Borrar");
		Nokia5110_SetCursor(0,5);	
		Nokia5110_OutString("usuario");
		
		//MENU SELECCION INICIO
		confirmar=0;
		while(confirmar!=0x0D){
			opcion=confirmar;
			while((opcion!='1')&&(opcion!='2')&&(opcion!='3')){
				opcion=rxcar_uart2();
			}
			confirmar=rxcar_uart2();
		}		
/**********************************************/
/***********ETAPA3: Ingreso de clave***********/	
/**********************************************/				
		if(opcion=='1'){ //Opcion 1: Ingresar clave
			//Solicitando clave
			Nokia5110_Clear();
			Nokia5110_OutString("Ingrese su");
			Nokia5110_SetCursor(0,1);
			Nokia5110_OutString("Clave");		
			Nokia5110_SetCursor(0,2);	
			txmens_uart2(mClave);
			//Ingresando
			input(&ingreso[0],&longPalabra,0,5);
			
			//Reset de variables de verificacion
			usuario=0;
			for( k=0; k < 11; k++ ){	
				verificar[k]=1;}
			
			//Verificacion de clave
			if(longPalabra>5){usuario=0;}
			else{				
				for( k=0; k < 11; k++ ){	
					for( i=0; i < 5; i++ ){		
						if(ingreso[i]!=claves[k][i]){
							verificar[k]=0;}}
					if(verificar[k]==1){
						usuario=k+1;} //Asignando clave a usuario+1 cuya clave corresponda
				}
			}	

/**********************************************/
/***********ETAPA4: Dar/Negar Acceso***********/	
/**********************************************/					
			if(usuario>=1){ //Caso clave valida para algun usuario
				//UART
				txmens_uart2(mFin1);
				txmens_uart2(usuarios[usuario-1]);
				//LCD
				Nokia5110_Clear();
				Nokia5110_OutString("Bienvenido");
			  Nokia5110_SetCursor(0,1);
			  Nokia5110_OutString((char*)usuarios[usuario-1]);	
				color=0x08; //Parpadeo final Verde
			}else{ //Caso clave invalida
				//UART
				txmens_uart2(mFin2);
				//LCD
				Nokia5110_Clear();
				Nokia5110_OutString("Clave");
			  Nokia5110_SetCursor(0,1);
				Nokia5110_OutString("Incorrecta");
				color=0x02;	//Parpadeo final Rojo
			}
		
/*******************************************************/
/**ETAPA5: Registro nuevo usuario (Espacio disponible)**/	
/*******************************************************/			
		}
		if(opcion=='2'){//Opcion 2: Registrar nuevo usuario
			//Ingresar clave maestra
			valida=input_maestra(maestra);			
		
			if(valida==1){ //Si clave maestra correcta
				if(ind<=9){ //Menos de 10 usuarios guardados
					do{//USUARIO
						nueva(&usuarios[ind][0],1);
						
						//Resetear verificacion
						usuario=0;
						for( k=0; k < 11; k++ ){verificar[k]=1;}
						
					//Verificar si coincide con alguno ya existente
						for( k=0; k < 11; k++ ){					
							if(k!=ind){
								for( i=0; i < 20; i++ ){		//Evaluar que coincidan sus caracteres
									if(usuarios[ind][i]!=usuarios[k][i]){
										verificar[k]=0;}
								}
								if(verificar[k]==1){usuario=k+1;} 								
							}
						}
						if(usuario!=0){
						txmens_uart2(mExistente);
						Nokia5110_Clear();
						Nokia5110_OutString("Usuario");
						Nokia5110_SetCursor(0,1);
						Nokia5110_OutString("ya existe");
						Parpadeo(0x02);
						}
					}while(usuario!=0);	//Si coincide con algun usuario seguira en el bucle			
					
					do{//CLAVE
						nueva(&claves[ind][0],0);
						
						//Resetear verificacion
						usuario=0;
						for( k=0; k < 11; k++ ){	
							verificar[k]=1;}
					//Verificar si coincide con alguno ya existente
						for( k=0; k < 11; k++ ){					
							if(k!=ind){
								for( i=0; i < 5; i++ ){		//Evaluar que coincidan sus caracteres
									if(claves[ind][i]!=claves[k][i]){
										verificar[k]=0;}
								}
								if(verificar[k]==1){usuario=k+1;} 	
							}
						}
						if(usuario!=0){
						txmens_uart2(mExistente);
						Nokia5110_Clear();
						Nokia5110_OutString("Clave");
						Nokia5110_SetCursor(0,1);
						Nokia5110_OutString("ya existe");
						Parpadeo(0x02);	
						}
					}while(usuario!=0);//Si coincide con algun usuario seguira en el bucle												
					
					//UART
					txmens_uart2(mUsuarioR);
					txmens_uart2(usuarios[ind]);
					//LCD
					Nokia5110_Clear();	
					Nokia5110_OutString("Se ha");
			    Nokia5110_SetCursor(0,1);
					Nokia5110_OutString("Registrado");
			    Nokia5110_SetCursor(0,2);
			    Nokia5110_OutString((char*)usuarios[ind]);		
					color=0x08; //Parpadeo final Verde
					ind++;
				}					
				
/*******************************************************/
/**ETAPA6: Registro nuevo usuario (Memoria llena)**/	
/*******************************************************/				
				else{		//Notificar que arreglos estan llenos
					//UART
					txmens_uart2(mLleno);
					//LCD
					Nokia5110_Clear();
					Nokia5110_OutString("Memoria");
			    Nokia5110_SetCursor(0,1);
					Nokia5110_OutString("llena");
					color=0x02;	//Parpadeo final Rojo  		
				}
			}				

/**********************************************/
/*******ETAPA7: Clave maestra incorrecta*******/	
/**********************************************/					
			else{
				txmens_uart2(mFin2);//Caso Clave maestra incorrecta
				//LCD
				Nokia5110_Clear();
        Nokia5110_OutString("Clave");
			  Nokia5110_SetCursor(0,1);
			  Nokia5110_OutString("incorrecta");
        color=0x02; //Parpadeo final Rojo  
			}
		}	

/**********************************************/
/************ETAPA8: Borrar Usuario************/	
/**********************************************/		
		if(opcion=='3'){//Opcion 3: Borrar usuario
			valida=input_maestra(maestra);
			
			if(valida==1){ //Si clave maestra correcta
			do{	
				color=0x08;
				txmens_uart2(mUsuarioS);
				//LCD
				Nokia5110_Clear();
				Nokia5110_OutString("Usuario a");
				Nokia5110_SetCursor(0,1);
				Nokia5110_OutString("borrar:");
				Nokia5110_SetCursor(0,2);
				//Ingreso
				input(&ingreso[0],&longUsuario,1,20);
							
				//Resetear verificacion
				usuario=0;
				for( k=0; k < 10; k++ ){verificar[k]=1;}
				
				//Verificar
				if((longUsuario<1)||(longUsuario>20)){usuario=0;}
				else{
					for( k=0; k < 10; k++ ){					
						for( i=0; i < 20; i++ ){//Evaluar que coincidan sus caracteres
							if(ingreso[i]!=usuarios[k][i]){
								verificar[k]=0;}
						}
						if(verificar[k]==1){usuario=k+1;} 
					}
				}

				if(usuario==0){ //Usuario invalido
					//LCD
					Nokia5110_Clear();
					Nokia5110_OutString("Usuario");
					Nokia5110_SetCursor(0,1);
					Nokia5110_OutString("invalido");
					//UART
					txmens_uart2(mUsuarioF);
					Parpadeo(0x02);}
				}while(usuario==0);
				
				//Remplazando credenciales de usuario seleccionado
				txmens_uart2(mUsuarioB);
				txmens_uart2(usuarios[usuario-1]);
				//LCD
				Nokia5110_Clear();
				Nokia5110_OutString("Borrando");
				Nokia5110_SetCursor(0,1);
				Nokia5110_OutString((char*)usuarios[usuario-1]);
					
				for(i=(usuario-1);i<9;i++){
					for(k=0;k<20;k++){
						if(k<5){claves[i][k]=claves[i+1][k];}
						usuarios[i][k]=usuarios[i+1][k];
					}
				}
				//Cambiando la ultima clave por la de admin otra vez
				for( k=0; k < 20; k++ ){	
					if(k<5){claves[9][k]=maestra[k];}
					usuarios[9][k]=0;
				}
				ind--; //Retrocediendo una posicion en los registros							
			}else{
				txmens_uart2(mFin2);//Caso Clave maestra incorrecta
				//LCD
				Nokia5110_Clear();
        Nokia5110_OutString("Clave");
			  Nokia5110_SetCursor(0,1);
			  Nokia5110_OutString("incorrecta");
        color=0x02;   
			}
		}			
		//Parpadeo final
		Parpadeo(color);
	}
}
/**********************************/
/**********FIN DE PROGRAMA*********/
/**********************************/
