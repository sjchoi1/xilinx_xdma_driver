# Xilinx xdma driver

## Build driver 
~~~
cd scripts
./build-install-driver-linux.sh
~~~

## Multi Scatter Gather DMA
~~~
cd XilinxAR65444/Linux/Xilinx_Answer_65444_Linux_Files/tests
make clean
make
./dma_to_device -a 0x80000000 -f file1 -f file2 -f file3 -f file4 -f file5 
~~~
