#!/bin/bash
addrStart='0x80000000'

cd /home/sjchoi/xilinx_xdma_driver/XilinxAR65444/Linux/Xilinx_Answer_65444_Linux_Files/tests
make clean
make

# Run the PCIe DMA memory mapped write read test
echo "[Info] Running PCIe DMA memory mapped write read test"
echo "      transfer size:  ${transferSize}"

echo "[Info] Writing to h2c channel 0 at address offset ${addrStart}."

echo ""
echo ""
echo "_______________________[TEST 1]______________________________"
echo "[Info] Testing Multi Scatter Gather DMA of size 4K * 100"
myvar=`perl -e "print '-f data/datafile0_4K.bin ' x 100;"`
./dma_to_device ${myvar} -a ${addrStart}

echo ""
echo ""
echo "_______________________[TEST 2]______________________________"
echo "[Info] Testing Multi Scatter Gather DMA of size 8K * 100"
myvar=`perl -e "print '-f data/datafile_8K.bin ' x 100;"`
./dma_to_device ${myvar} -a ${addrStart}

echo ""
echo ""
echo "_______________________[TEST 3]______________________________"
echo "[Info] Testing Memory Access Dump"
./memory_access_dump -a ${addrStart} -f memory_access_dump.bin
echo ""
echo ""
echo ""
echo ""

exit 0
