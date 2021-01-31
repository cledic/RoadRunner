
Si tratta di un "porting" dei driver ST per la IMU lsm6ds3, i cui sorgenti sono disponibili a questo link: https://github.com/STMicroelectronics/STMems_Standard_C_drivers 
per la board Linux RoadRunner della AcmeSystems. 

La ST rende disponibili anche i driver del kernel per Linux a questo link: https://github.com/STMicroelectronics/STMems_Linux_Input_drivers/tree/linux-3.10.y-gh/drivers/input/misc/st

I driver sono sotto il folder "driver" e non li ho modificati, mentre il file "lsm6ds3_read_data_polling.c" è tratto dagli esempi ed è modificato per le parti di READ/WRITE a basso livello con la SPI.

Il driver Linux SPI è lo "spidev" e per far comparire il driver sotto "/dev" bisogna modificare il DTS come da file "modifica a DTS per spidev.txt". Dopo questa modifica sotto "/dev" apparirà il file "spidev0.0"
