#ifndef _TNTACTILEJACKET_
#define _TNTACTILEJACKET_

#define TJACKET_LIB_VERSION		(1a)		/* Version 1a */
#define NUMBER_CELLS			(8)			/* Total number of cells in the jacket */
//Flush params
#define FLUSH_ALL		(-1) //Flush All Effects
//Errors...
#define GLIB_OK			(0) //no error
#define GLIB_NOTINIT	(1) //Not Initialized, Call SetUpJacket()
#define GLIB_BADPARAM	(2) //Bad Param passed to function
#define GLIB_ERR_COMM	(3) //Communication with jacket Failed
#define GLIB_NO_EFF		(4)	//Invalid Predefined Effect
//Pre-defined library effects
typedef enum {

	E_MACHINEGUN_FRONT,
	E_MACHINEGUN_BACK,
	E_BIG_BLAST_FRONT,
	E_BIG_BLAST_BACK,
	E_SMALL_BLAST_FRONT,
	E_SMALL_BLAST_BACK,
	E_PISTOL_FRONT,
	E_PUNCH_FRONT,
	E_STAB_FRONT,
	E_SHOTGUN_FRONT,
	E_RIFLE_FRONT,
	E_PISTOL_BACK,
	E_PUNCH_BACK,
	E_STAB_BACK,
	E_SHOTGUN_BACK,
	E_RIFLE_BACK,
	E_LEFT_SIDE_HIT,
	E_RIGHT_SIDE_HIT,
	E_ACCELERATION,
	E_DECELERATION,
	E_LEFTTURN,
	E_RIGHTTURN,
	E_ACCELERATION_STOP,
	E_DECELERATION_STOP,
	E_LEFTTURN_STOP,
	E_RIGHTTURN_STOP

}TN_JEFFECTS;

#ifdef __cplusplus
extern "C" {
#endif


/* 
	int SetUpJacket(void);

	Set up the tactile jacket 
	
	Must be called before any commands can be sent
*/
extern int __stdcall SetUpJacket();

/* 
	int TearDownJacket(void);

	Clean up the tactile jacket 

	Deallocate any resources or memory used in this library
*/
extern void __stdcall TearDownJacket();


/* 
	int SetEffect(int nEffect); 

	Set a pre-defined effect 
	
	See TN_JEFFECTS (above)

*/
extern int __stdcall SetEffect(int nEffect); 

/* 

	int SetEffect2(int speed,int actuator); 
  
	Set a custom effect 
	
	param 1: Effect Speed ... 1(fast) - 10(slow)
	param 2: Actuator 1 - 8

*/
extern int __stdcall SetEffect2(int speed,int actuator); 

/* 

	void FlushBuffer(int actuator); 
  
	Clears all effects on the actuators buffer 
	
	param 1: Actuator 1 - 8 or FLUSH_ALL to flush all buffers

*/
extern void __stdcall FlushBuffer(int actuator); 



/* 

	int GetErrorCode(void); 
  
	Returns the numeric value of the last error.

*/
extern int __stdcall GetErrorCode();

/* 

	char* GetErrorText(void);
  
	Returns the text description of the last error.

*/
extern char* __stdcall GetErrorText();


#ifdef __cplusplus
}
#endif

#endif /* _TNTACTILEJACKET_ */