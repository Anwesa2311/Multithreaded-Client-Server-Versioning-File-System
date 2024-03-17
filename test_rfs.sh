#!/bin/bash

# Starting the server in the background
./rfserver &
SERVER_PID=$!

# Waiting for the server to start
sleep 1

# Test Case 1:- WRITE COMMAND
echo "Testing WRITE command..."
./rfs WRITE Client/Client.txt Server/server.txt
echo "Test case 1 completed."
echo "*******************************************************"

#Test Case 2:- Write Command when no directory exists
echo "Testing WRITE command when no directory exists..."
./rfs WRITE Client/Client.txt newFolder/data.txt
echo "Test case 2 completed."
echo "*******************************************************"


#Test Case 3:- Write command when only local path provided

#Test Case 4:- Write command with versioning
echo "Testing WRITE command with versioning..."
./rfs WRITE Client/Demo.txt Server/server.txt

if [ -f "Server/server.txt" ] && [ -f "newFolder/data.txt"]; then
    echo "WRITE tests passed: Files written successfully."
else
    echo "WRITE tests failed: Files not written successfully."
fi
echo "Test case 3:-WRITE test with versioning completed."
echo "*******************************************************"

# Test GET command

#Test case 5:- GET command without versioning and all paths provided
echo "Testing GET command without versioning and all paths provided..."
./rfs GET Server/file.txt Client/retrieved_file.txt

if [ -f "Client/retrieved_file.txt" ]; then
    echo "GET tests passed: File retrieved from server successfully."
else
    echo "GET tests failed: Files could not be retrieved from server."
fi
echo "Test case 5 completed."

echo "*******************************************************"

#Test case 6:- GET command with versioning
echo "Testing GET command with versioning..."
./rfs GET Server/file.txt Client/Version.txt 20231208201750
if [ -f "Client/Version.txt" ] ; then
    echo "WRITE tests passed: Files written successfully."
else
    echo "WRITE tests failed: Files not written successfully."
fi


echo "Test case 6 completed."

echo "*******************************************************"


#Test case 7:- GET command without versioning and only local file provided
echo "Testing GET command without versioning and only remote file provided..."
./rfs GET Sever/file.txt
echo "Test case 7 completed."

if [ -f "Client/retrieved_file.txt" ]; then
    echo "GET test passed: Files retrieved successfully."
else
    echo "GET test failed: Files not retrieved."
fi
echo "*******************************************************\n"

# Test LS command
#Test case 8:- LS command 
echo "Testing LS command..."
./rfs LS  Server/filebackup20231208201750.txt > ls_output.txt
if [ -s "ls_output.txt" ]; then
    echo "LS test passed: Listing output found."
else
    echo "LS test failed: No listing output."
fi

echo "Test case 8 completed...."

echo "*******************************************************\n"
#Test RM command
#Test case 9:- Test RM command with versioning
echo "Testing RM command..."
./rfs WRITE Client/Client.txt Server/RM.txt
./rfs WRITE Client/Client.txt Server/RMbackup213456.txt
./rfs WRITE Client/Client.txt Server/RMbackup201345.txt
./rfs RM Server/RM.txt
echo "RM test completed."
echo "RM test for non-existing file completed."
# Allow brief delay for server processing
sleep 2

# Check if the file and its versions are deleted
if [ -f Server/RM.txt ] && [ -f Server/RMbackup213456.txt ] && [ -f Server/RMbackup201345.txt ]; then
echo "Original file with all versions sucessfully deleted.."
else

    echo "Test case 9 completed."

    fi

echo "*******************************************************"

# Test GET command after the target file has been deleted
echo "Testing GET command after file deletion..."
./rfs GET Server/RM.txt
if [ -f "Server/RM.txt" ]; then
    echo "GET test after delete failed: File should not exist."
else
    echo "GET test after delete passed: File does not exist as expected."
fi

echo "Test case 10 completed"

echo "*******************************************************"

# Test the behavior of WRITE command with a large file
echo "Testing WRITE command with a large file..."
dd if=/dev/urandom of=data/largefile.txt bs=1M count=10  # Create a 10MB file
./rfs WRITE data/largefile.txt folder/largefile.txt
echo "WRITE test with a large file completed."

echo "Test case 11 completed"

echo "*******************************************************"

# Test STOP command
echo "Testing STOP command..."
./rfs STOP
echo "STOP test completed."

echo "Test case 12 completed"

echo "*******************************************************"

# Allow brief delay for the server to process the STOP command
sleep 2

# Check if the server process has stopped
if ! ps -p $SERVER_PID > /dev/null; then
   echo "STOP test passed: Server stopped successfully."
else
   echo "STOP test failed: Server is still running."
fi



echo "Testing 12/12 sucessfully completed."
