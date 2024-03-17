#!/bin/bash

# Start the server in the background
./rfserver &
SERVER_PID=$!

# Allow some time for the server to start
sleep 1

# Test multiple client connections
echo "Testing multiple client connections..."

# Run several client commands in the background
./rfs WRITE Client/Client.txt folder/foo1.txt &
PID1=$!
./rfs WRITE Client/Demo.txt folder/foo2.txt &
PID2=$!
./rfs GET folder/foo1.txt Client/retrieved_foo1.txt &
PID3=$!
./rfs GET folder/foo2.txt Client/retrieved_foo2.txt &
PID4=$!

# Wait for all background processes to finish
wait $PID1 $PID2 $PID3 $PID4

# Check if all operations were successful
if [ -f folder/foo1.txt ] && [ -f folder/foo2.txt ] && [ -f Client/retrieved_foo1.txt ] && [ -f Client/retrieved_foo2.txt ]; then
    echo "Multiple client connection test passed: All operations completed successfully."
else
    echo "Multiple client connection test failed: Some operations did not complete as expected."
fi


# Stopping the server
kill -SIGINT $SERVER_PID
wait $SERVER_PID

echo "Testing completed."
