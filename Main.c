/*

This a LSS Configurator to load pin.

Copyright (C): Héctor ASCORBE and Sergio RODRÍGUEZ.

*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <sys/time.h>

#include "canfestival.h"
#include "Main.h"
#include "LoadPin.h"
#include "siecalg.h"

unsigned int masterNodeId = 0x01; /* 1 */
unsigned int slaveNodeId = 0x65; /* 127 */

#if !defined(WIN32) || defined(__CYGWIN__)
void catch_signal(int sig)
{
  signal(SIGTERM, catch_signal);
  signal(SIGINT, catch_signal);

  printf("Got Signal %d\n",sig);
}
#endif

static void CheckSDOAndContinue(CO_Data* d, UNS8 nodeId, void* value, UNS32 length)
{
	UNS32 abortCode;

	while(getReadResultNetworkDict (&Load_Pin_Data, slaveNodeId, value, &length, &abortCode) != SDO_FINISHED);

}

void configureNodes(CO_Data* d){
    //It is assumed correct settings

	// Master sets in operational mode
	setState(&Load_Pin_Data, Operational);

	// We ask to slave to set in operational mode too
	UNS8 err = masterSendNMTstateChange (&Load_Pin_Data, slaveNodeId, NMT_Start_Node);

	if (err)
	{
		printf("Error putting operational mode in slave: %d\n", err);
	}

}

/***************************  INIT  *****************************************/
void InitNodes(CO_Data* d, UNS32 id)
{
	setNodeId(&Load_Pin_Data, masterNodeId);
}

/***************************  EXIT  *****************************************/
void Exit(CO_Data* d, UNS32 id)
{
	setState(&Load_Pin_Data, Stopped);
}

//Callbacks functions declaration

void Load_Pin_heartbeatError(CO_Data* d, UNS8 heartbeatID)
{
	printf("Error heartbeat %d\n", heartbeatID);
}

void Load_Pin_initialisation(CO_Data* d){
	printf("Master node initialization\n");
}

void Load_Pin_preOperational(CO_Data* d){
	printf("Master in PreOperational mode\n");
}

void Load_Pin_operational(CO_Data* d){
	printf("Master in Operational mode\n");
}

void Load_Pin_stopped(CO_Data* d){
	printf("Master in Stopped mode\n");
}

void Load_Pin_post_sync(CO_Data* d)
{
	printf("Sync object posted in Bus\n");
}

void Load_Pin_post_TPDO(CO_Data* d)
{
	printf("Posted Transmission PDO in busP\n");
}

//End callbacks functions declaration

static void CheckLSSAndContinue(CO_Data* d, UNS8 command);
static void ConfigureLSSNode(CO_Data* d);

//Callback funcition to get result of configuration. Follow state machine according to framework, by flags.
static void CheckLSSAndContinue(CO_Data* d, UNS8 command)
{
	UNS32 dat1;
	UNS8 dat2;

	printf("CheckLSS\n");
	if(getConfigResultNetworkNode (d, command, &dat1, &dat2) != LSS_FINISHED){
			printf("Master : Failed in LSS comand %d.  Trying again\n", command);
	}
	else
	{
		//init_step_LSS++;

		switch(command){
		case LSS_CONF_NODE_ID:
   			switch(dat1){
   				case 0: printf("Node ID change succesful\n");break;
   				case 1: printf("Node ID change error:out of range\n");break;
   				case 0xFF:printf("Node ID change error:specific error\n");break;
   				default:break;
   			}
   			break;
   		case LSS_CONF_BIT_TIMING:
   			switch(dat1){
   				case 0: printf("Baud rate change succesful\n");break;
   				case 1: printf("Baud rate change error: change baud rate not supported\n");break;
   				case 0xFF:printf("Baud rate change error:specific error\n");break;
   				default:break;
   			}
   			break;
   		case LSS_CONF_STORE:
   			switch(dat1){
   				case 0: printf("Store configuration succesful\n");break;
   				case 1: printf("Store configuration error:not supported\n");break;
   				case 0xFF:printf("Store configuration error:specific error\n");break;
   				default:break;
   			}
   			break;
   		case LSS_CONF_ACT_BIT_TIMING:
   			if(dat1==0){
   				UNS8 LSS_mode=LSS_WAITING_MODE;
				UNS32 SINC_cicle=50000;// us
				UNS32 size = sizeof(UNS32);

				/* The slaves are now configured (nodeId and Baudrate) via the LSS services.
   			 	* Switch the LSS state to WAITING and restart the slaves. */


	   			printf("Master : Switch Delay period finished. Switching to LSS WAITING state\n");
   				configNetworkNode(d,LSS_SM_GLOBAL,&LSS_mode,0,NULL);

   				printf("Master : Restarting all the slaves\n");
   				masterSendNMTstateChange (d, 0x00, NMT_Reset_Comunication);

   				printf("Master : Starting the SYNC producer\n");
   				writeLocalDict( d, /*CO_Data* d*/
					0x1006, /*UNS16 index*/
					0x00, /*UNS8 subind*/
					&SINC_cicle, /*void * pSourceData,*/
					&size, /* UNS8 * pExpectedSize*/
					RW);  /* UNS8 checkAccess */

				return;
			}
   			else{
   				UNS16 Switch_delay=1;
				UNS8 LSS_mode=LSS_CONFIGURATION_MODE;

	   			eprintf("Master : unable to activate bit timing. trying again\n");
				configNetworkNode(d,LSS_CONF_ACT_BIT_TIMING,&Switch_delay,0,CheckLSSAndContinue);
				return;
   			}
   			break;
		case LSS_SM_SELECTIVE_SERIAL:
   			printf("Slave in LSS CONFIGURATION state\n");
   			break;
   		case LSS_IDENT_REMOTE_SERIAL_HIGH:
   			printf("Node identified\n");
   			break;
   		case LSS_IDENT_REMOTE_NON_CONF:
   			if(dat1==0)
   				eprintf("There are no-configured remote slave(s) in the net\n");
   			else
   			{
   				UNS16 Switch_delay=1;
				UNS8 LSS_mode=LSS_CONFIGURATION_MODE;

				/*The configuration of the slaves' nodeId ended.
				 * Start the configuration of the baud rate. */
				eprintf("Master : There are not no-configured slaves in the net\n", command);
				eprintf("Switching all the nodes to LSS CONFIGURATION state\n");
				configNetworkNode(d,LSS_SM_GLOBAL,&LSS_mode,0,NULL);
				eprintf("LSS=>Activate Bit Timing\n");
				configNetworkNode(d,LSS_CONF_ACT_BIT_TIMING,&Switch_delay,0,CheckLSSAndContinue);
				return;
   			}
   			break;
   		case LSS_INQ_VENDOR_ID:
   			printf("Slave VendorID %x\n", dat1);
   			break;
   		case LSS_INQ_PRODUCT_CODE:
   			printf("Slave Product Code %x\n", dat1);
   			break;
   		case LSS_INQ_REV_NUMBER:
   			printf("Slave Revision Number %x\n", dat1);
   			break;
   		case LSS_INQ_SERIAL_NUMBER:
   			printf("Slave Serial Number %x\n", dat1);
   			break;
   		case LSS_INQ_NODE_ID:
   			printf("Slave nodeid %x\n", dat1);
   			break;
#ifdef CO_ENABLE_LSS_FS
   		case LSS_IDENT_FASTSCAN:
   			if(dat1==0)
   				printf("Slave node identified with FastScan\n");
   			else
   			{
   				printf("There is not unconfigured node in the net\n");
   				return;
   			}
   			//init_step_LSS++;
   			break;
#endif

		}
	}

	printf("\n");
	//ConfigureLSSNode(d);
}

void configureNodeByLSS(CO_Data* objetCAN, UNS8 nuevoIdNodo, char* baudrate){
	printf("------------------LSS Configuration------------------\n");

	UNS8 data1, data2; //Puntero a datos opcionales a pasar, que dependen del comando.

	UNS8 result;

	printf("Inicio del proceso de configuracion \n");

	//1.- Identify Slave node

	if(result)
		printf("Error in first step, code: %x\n", result);

	UNS32 Vendor_ID=0x164;
	UNS32 Product_Code=0x14998E;
	UNS32 Revision_Number=0x3040306;
	UNS32 Serial_Number=0x3D680D;
	UNS32 Revision_Number_high;
	UNS32 Revision_Number_low;
	UNS32 Serial_Number_high;
	UNS32 Serial_Number_low;

	// LSS_CONFIGURATION_MODE

	data1=0;

	printf("Asking for nodeID\n");
	result = configNetworkNode(&Load_Pin_Data,LSS_SM_GLOBAL,&data1,&data2,CheckLSSAndContinue);
	if(result)
		printf("Error first step, code: %x\n", result);

	printf("Asking for nodeID\n");

	sleep(1);

	data1=1;

	printf("Asking for nodeID\n");
	result = configNetworkNode(&Load_Pin_Data,LSS_SM_GLOBAL,&data1,&data2,CheckLSSAndContinue);
	if(result)
		printf("Error in second step, code: %x\n", result);

	printf("Asking for nodeID\n");

	sleep(1);

	data1=0x65;

	if(!result)
		result=configNetworkNode(&Load_Pin_Data,LSS_CONF_NODE_ID,&data1,&data2,CheckLSSAndContinue);
	if(result)
		printf("Error in third step, code: %x\n", result);

	sleep(1);

	data1=0;

	printf("Asking for nodeID\n");
	result = configNetworkNode(&Load_Pin_Data,LSS_CONF_STORE,&data1,&data2,CheckLSSAndContinue);
	if(result)
		printf("Error in fourthstep , code: %x\n", result);

	printf("Asking for nodeID\n");

	sleep(1);

	data1=0;

	printf("Asking for nodeID\n");
	result = configNetworkNode(&Load_Pin_Data,LSS_SM_GLOBAL,&data1,&data2,CheckLSSAndContinue);
	if(result)
		printf("Error in fifth step, code: %x\n", result);

	printf("Asking for nodeID\n");

}

/* Initial nodeID and VendorID. They are incremented by one for each slave*/
UNS8 NodeID=0x0100;
UNS32 Vendor_ID=0x164;

/* Configuration of the nodeID and baudrate with LSS services:
 * --First ask if there is a node with an invalid nodeID.
 * --If FastScan is activated it is used to put the slave in the state “configuration”.
 * --If FastScan is not activated, identification services are used to identify the slave. Then
 * 	 switch mode service is used to put it in configuration state.
 * --Next, all the inquire services are used (only for example) and a valid nodeId and a
 * 	 new baudrate are assigned to the slave.
 * --Finally, the slave's LSS state is restored to “waiting” and all the process is repeated
 * 	 again until there isn't any node with an invalid nodeID.
 * --After the configuration of all the slaves finished the LSS state of all of them is switched
 * 	 again to "configuration" and the Activate Bit Timing service is requested. On sucessfull, the
 * 	 LSS state is restored to "waiting" and NMT state is changed to reset (by means of the NMT services).
 * */
static void ConfigureLSSNode(CO_Data* d)
{
	printf("------------------Configuracion LSS------------------\n");
	//Requisitos: conexion 1:1, ser soportado por el esclavo el servicio LSS.

	UNS8 dat1, dat2;

	UNS32 Vendor_ID=0x164;
	UNS32 Product_Code=0x14998E;
	UNS32 Revision_Number=0x3040306;
	UNS32 Serial_Number=0x3D680D;
	UNS32 Revision_Number_high;
	UNS32 Revision_Number_low;
	UNS32 Serial_Number_high;
	UNS32 Serial_Number_low;
	UNS8 LSS_mode=LSS_WAITING_MODE;
	UNS8 Baud_Table=3; //According to table
	//UNS8 Baud_BitTiming=3;
	char* Baud_BitTiming="250K";
	UNS8 res;

	printf("LSS Configuration Starting \n");

	//1.- Remote slave node identification

	res=configNetworkNode(&Load_Pin_Data,LSS_IDENT_REMOTE_NON_CONF,&dat1,&dat2,CheckLSSAndContinue);
	if(res)
		printf("Error in first step, code: %x\n", res);

#ifdef CO_ENABLE_LSS_FS
	//2.-  LSS=>FastScan
		lss_fs_transfer_t lss_fs;
		printf("LSS=>FastScan\n");
		/* The VendorID and ProductCode are partialy known, except the last two digits (8 bits). */
		lss_fs.FS_LSS_ID[0]=Vendor_ID;
		lss_fs.FS_BitChecked[0]=8;
		lss_fs.FS_LSS_ID[1]=Product_Code;
		lss_fs.FS_BitChecked[1]=8;
		/* serialNumber and RevisionNumber are unknown, i.e. the 8 digits (32bits) are unknown. */
		lss_fs.FS_BitChecked[2]=32;
		lss_fs.FS_BitChecked[3]=32;
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_IDENT_FASTSCAN,&lss_fs,0,CheckLSSAndContinue);
		if(res)
			printf("Error in second step, code: %x\n", res);
#else
	//2.- Node identification
		printf("Node Identification\n");
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_IDENT_REMOTE_VENDOR,&Vendor_ID,0,NULL);
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_IDENT_REMOTE_PRODUCT,&Product_Code,0,NULL);
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_IDENT_REMOTE_REV_LOW,&Revision_Number_low,0,NULL);
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_IDENT_REMOTE_REV_HIGH,&Revision_Number_high,0,NULL);
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_IDENT_REMOTE_SERIAL_LOW,&Serial_Number_low,0,NULL);
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_IDENT_REMOTE_SERIAL_HIGH,&Serial_Number_high,0,CheckLSSAndContinue);
		if(res)
		printf("Error in second step, code: %x\n", res);

	//3.- Putting slave node in configuration mode
		printf("Putting slave node in configuration mode\n");
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_SM_SELECTIVE_VENDOR,&Vendor_ID,0,NULL);
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_SM_SELECTIVE_PRODUCT,&Product_Code,0,NULL);
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_SM_SELECTIVE_REVISION,&Revision_Number,0,NULL);
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_SM_SELECTIVE_SERIAL,&Serial_Number,0,CheckLSSAndContinue);
		//Vendor_ID++; //Ver que hacer con esto
		if(res)
			printf("Error en paso 3, codigo: %x\n", res);
#endif
	//4.- Asking for nodeID
		printf("Asking for nodeID\n");
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_INQ_NODE_ID,0,0,CheckLSSAndContinue);
		if(res)
			printf("Error en paso 4, codigo: %x\n", res);

	//5.- Preguntar VendorID
		printf("Preguntar VendorID\n");
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_INQ_VENDOR_ID,0,0,CheckLSSAndContinue);
		if(res)
			printf("Error en paso 5, codigo: %x\n", res);

	//6.- preguntar Product code
		printf("Preguntar Product code\n");
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_INQ_PRODUCT_CODE,0,0,CheckLSSAndContinue);
		if(res)
			printf("Error en paso 6, codigo: %x\n", res);

	//7.- Preguntar Revision Number
		printf("Preguntar Revision Number\n");
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_INQ_REV_NUMBER,0,0,CheckLSSAndContinue);
		if(res)
			printf("Error en paso 7, codigo: %x\n", res);

	//8.- Preguntar Serial Number
		printf("Preguntar Serial Number\n");
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_INQ_SERIAL_NUMBER,0,0,CheckLSSAndContinue);
		if(res)
			printf("Error en paso 8, codigo: %x\n", res);

	//9.- Cambiar el nodeID
		printf("Cambiar el nodeId\n");
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_CONF_NODE_ID,&NodeID,0,CheckLSSAndContinue);
		//NodeID++;
		if(res)
			printf("Error en paso 9, codigo: %x\n", res);

	//10.- Cambiar tasa de transferencia
		printf("Cambiar tasa de transferencia\n");
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_CONF_BIT_TIMING,&Baud_Table,&Baud_BitTiming,CheckLSSAndContinue);
		if(res)
			printf("Error en paso 10, codigo: %x\n", res);

	//11.- Almacenar configuracion
		printf("Almacenar configuracion\n");
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_CONF_STORE,0,0,CheckLSSAndContinue);
		if(res)
			printf("Error en paso 11, codigo: %x\n", res);

	//12.- Poner en modo de espera
		printf("Poner en modo de espera\n");
		if(!res)
			res=configNetworkNode(&Load_Pin_Data,LSS_SM_GLOBAL,&LSS_mode,0,NULL);
		if(res)
			printf("Error en paso 12, codigo: %x\n", res);

	printf("------------------Fin Config. LSS------------------\n");
}


int main(int argc,char **argv)
{

	printf("WELCOME TO LSS CONFIGURATOR\n");

	TimerInit();

	LoadCanDriver("/usr/local/lib/libcanfestival_can_socket.so");

	s_BOARD MasterBoard = {"mxcan1", "250K"};

	canOpen(&MasterBoard,&Load_Pin_Data);

	//Callbacks function definitions
	Load_Pin_Data.heartbeatError = Load_Pin_heartbeatError;
	Load_Pin_Data.initialisation = Load_Pin_initialisation;
	Load_Pin_Data.preOperational = Load_Pin_preOperational;
	Load_Pin_Data.operational = Load_Pin_operational;
	Load_Pin_Data.stopped = Load_Pin_stopped;
	Load_Pin_Data.post_sync = Load_Pin_post_sync;
	Load_Pin_Data.post_TPDO = Load_Pin_post_TPDO;

	// Start timer thread
	StartTimerLoop(&InitNodes);

	configureNodes(&Load_Pin_Data); //Establece el estado operacional en maestro y esclavo.

	UNS8 dataType;

	UNS8 err = readNetworkDict (&Load_Pin_Data, slaveNodeId, 0x7130, 1, 0, 0);

	UNS16 lectura;
	UNS32 length = sizeof(lectura);

	UNS32 abortCode;

	while(getReadResultNetworkDict (&Load_Pin_Data, slaveNodeId, &lectura, &length, &abortCode) != SDO_FINISHED);

	printf("El valor de lectura es: %x\n", lectura);

	// Stop timer thread
	StopTimerLoop(&Exit);

	printf("FIN\n");

	canClose(&Load_Pin_Data);

	TimerCleanup();

	return 0;

}
