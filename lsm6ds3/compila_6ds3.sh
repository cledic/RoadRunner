gcc -o lsm6ds3_tst lsm6ds3_tst.c lsm6ds3_read_data_polling.c spi.c ./driver/lsm6ds3_reg.c -I ./ -I ./driver/ -lexplain
