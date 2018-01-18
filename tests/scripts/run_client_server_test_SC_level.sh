#!/bin/bash
WORKSPACE_DIR=.
if [[ -n $1 ]]; then
    EXEC_DIR=$1
else
    EXEC_DIR=$WORKSPACE_DIR/bin
fi

cd $EXEC_DIR
# Create script for running stub_server in background and store exit code
echo "#!/bin/bash
./test_secure_channels_server \$1
echo \$? > server.exitcode" > test_server.sh
chmod +x test_server.sh

echo ""
echo "======================================================================"
echo "Secure channels tests level: SignAndEncrypt B256Sha256 2048 key length"
echo "======================================================================"

## SC tests: SignAndEncrypt
# remove precedent exit code file
rm -f server.exitcode
# Execute server side of the test
./test_server.sh &
sleep 1 # Wait server started

# Execute client side of the test and retrieve exit code
./test_secure_channels_client
CLIENT_EXITCODE="$?"
# Wait end of server side execution and retrieve exit code
wait
SERVER_EXITCODE=`cat server.exitcode`

# Fullfil TAP result
if [[ $CLIENT_EXITCODE -eq 0 && $SERVER_EXITCODE -eq 0 ]]; then
    echo "ok 1 - test: test_secure_channels_client / test_secure_channels_server: Passed" > client_server_result.tap
else
    echo "not ok 1 - test: test_secure_channels_client / test_secure_channels_server exit codes: $CLIENT_EXITCODE / $SERVER_EXITCODE" > client_server_result.tap
fi

echo ""
echo "======================================================================"
echo "Secure channels tests level: Sign B256Sha256 2048 key length"
echo "======================================================================"

## SC tests: Sign
# remove precedent exit code file
rm -f server.exitcode
# Execute server side of the test
./test_server.sh &
sleep 1 # Wait server started

# Execute client side of the test and retrieve exit code
./test_secure_channels_client sign
CLIENT_EXITCODE="$?"
# Wait end of server side execution and retrieve exit code
wait
SERVER_EXITCODE=`cat server.exitcode`

# Fullfil TAP result
if [[ $CLIENT_EXITCODE -eq 0 && $SERVER_EXITCODE -eq 0 ]]; then
    echo "ok 2 - test: test_secure_channels_client sign / test_secure_channels_server: Passed" >> client_server_result.tap
else
    echo "not ok 2 - test: test_secure_channels_client sign / test_secure_channels_server exit codes: $CLIENT_EXITCODE / $SERVER_EXITCODE" >> client_server_result.tap
fi

echo ""
echo "======================================================================"
echo "Secure channels tests level: None"
echo "======================================================================"

## SC tests: None
# remove precedent exit code file
rm -f server.exitcode
# Execute server side of the test
./test_server.sh &
sleep 1 # Wait server started

# Execute client side of the test and retrieve exit code
./test_secure_channels_client none
CLIENT_EXITCODE="$?"
# Wait end of server side execution and retrieve exit code
wait
SERVER_EXITCODE=`cat server.exitcode`

# Fullfil TAP result
if [[ $CLIENT_EXITCODE -eq 0 && $SERVER_EXITCODE -eq 0 ]]; then
    echo "ok 3 - test: test_secure_channels_client none / test_secure_channels_server: Passed" >> client_server_result.tap
else
    echo "not ok 3 - test: test_secure_channels_client none / test_secure_channels_server exit codes: $CLIENT_EXITCODE / $SERVER_EXITCODE" >> client_server_result.tap
fi

echo ""
echo "======================================================================"
echo "Secure channels tests level: Sign B256 2048 key length"
echo "======================================================================"

## SC tests: SignAndEncrypt B256
# remove precedent exit code file
rm -f server.exitcode
# Execute server side of the test
./test_server.sh &
sleep 1 # Wait server started

# Execute client side of the test and retrieve exit code
./test_secure_channels_client encrypt B256
CLIENT_EXITCODE="$?"
# Wait end of server side execution and retrieve exit code
wait
SERVER_EXITCODE=`cat server.exitcode`

# Fullfil TAP result
if [[ $CLIENT_EXITCODE -eq 0 && $SERVER_EXITCODE -eq 0 ]]; then
    echo "ok 4 - test: test_secure_channels_client encrypt B256 / test_secure_channels_server: Passed" >> client_server_result.tap
else
    echo "not ok 4 - test: test_secure_channels_client encrypt B256 / test_secure_channels_server exit codes: $CLIENT_EXITCODE / $SERVER_EXITCODE" >> client_server_result.tap
fi

echo ""
echo "======================================================================"
echo "Secure channels tests level: SignAndEncrypt B256Sha256 4096 key length"
echo "======================================================================"

## SC tests: SignAndEncrypt B256Sha256 4096 key length
# remove precedent exit code file
rm -f server.exitcode
# Execute server side of the test
./test_server.sh 4096 &
sleep 1 # Wait server started

# Execute client side of the test and retrieve exit code
./test_secure_channels_client encrypt B256Sha256 4096
CLIENT_EXITCODE="$?"
# Wait end of server side execution and retrieve exit code
wait
SERVER_EXITCODE=`cat server.exitcode`

# Fullfil TAP result
if [[ $CLIENT_EXITCODE -eq 0 && $SERVER_EXITCODE -eq 0 ]]; then
    echo "ok 5 - test: test_secure_channels_client encrypt B256Sha256 4096 / test_secure_channels_server 4096: Passed" >> client_server_result.tap
else
    echo "not ok 5 - test: test_secure_channels_client encrypt B256Sha256 4096 / test_secure_channels_server 4096 exit codes: $CLIENT_EXITCODE / $SERVER_EXITCODE" >> client_server_result.tap
fi


echo "1..5" >> client_server_result.tap

# Clean created files
rm -f server.exitcode test_server.sh

