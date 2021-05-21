#!/bin/bash

addrStart='0x80000000'

testError=0
# Run the PCIe DMA memory mapped write read test
echo "[Info] Running PCIe DMA memory mapped write read test"
echo "      transfer size:  ${transferSize}"

echo "[Info] Writing to h2c channel 0 at address offset ${addrStart}."


echo ""
echo ""
echo "_______________________[TEST 1]______________________________"
echo "[Info] Testing Multi Scatter Gather DMA of size 4K * 100"
myvar=`perl -e "print '-f data/datafile0_4K.bin ' x 100;"`
./dma_to_device ${myvar} -s 4096 -a ${addrStart}

echo ""
echo ""
echo "_______________________[TEST 2]______________________________"
echo "[Info] Testing Multi Scatter Gather DMA of size 8K * 100"
myvar=`perl -e "print '-f data/datafile_8K.bin ' x 100;"`
./dma_to_device ${myvar} -s 8192 -a ${addrStart}

echo ""
echo ""
echo "_______________________[TEST 3]______________________________"
transferSize=(4096*256)
echo "[Info] Testing Multi Scatter Gather DMA of size 32K * 100"
myvar=`perl -e "print '-f data/datafile_256K.bin ' x 100;"`
./dma_to_device ${myvar} -s 32768 -a ${addrStart}

echo ""
echo ""
echo ""
echo ""

exit 0
