#!/bin/bash

transferSize='4096'
addrStart='0x80000000'
receiveSize='16384' 

testError=0
# Run the PCIe DMA memory mapped write read test
echo "Info: Running PCIe DMA memory mapped write read test"
echo "      transfer size:  ${transferSize}"

echo "Info: Writing to h2c channel 0 at address offset ${addrStart}."
./dma_to_device -d /dev/xdma0_h2c_0 \
    -f data/datafile0_4K.bin \
    -f data/datafile1_4K.bin \
    -f data/datafile2_4K.bin \
    -f data/datafile3_4K.bin \
    -f data/datafile4_4K.bin \
    -f data/datafile5_4K.bin \
    -f data/datafile6_4K.bin \
    -f data/datafile7_4K.bin \
    -f data/datafile8_4K.bin \
    -f data/datafile9_4K.bin \
    -s ${transferSize} -a ${addrStart}

rm -f data/output_datafile0_4K.bin
echo "Info: Reading from c2h channel 0 at address offset $addrStart."
./dma_from_device -d /dev/xdma0_c2h_0 \
    -f data/output_datafile0_4K.bin \
    -s ${receiveSize} -a ${addrStart}

# Verify that the written data matches the read data if possible.
echo "Info: Checking data integrity."

cmp data/output_datafile0_4K.bin data/datafile0_4K.bin -n $transferSize
returnVal=$?
if [ ! $returnVal == 0 ]; then
echo "Error: The data written did not match the data that was read."
testError=1
else
echo "Info: Data check passed"
fi

# Exit with an error code if an error was found during testing
if [ $testError -eq 1 ]; then
  echo "Error: Test completed with Errors."
  exit 1
fi

# Report all tests passed and exit
echo "Info: All PCIe DMA memory mapped tests passed."
exit 0
