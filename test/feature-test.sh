#/bin/sh
sudo make install
set -e
./test/launch-hatohol-for-test.sh
LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH hatohol-db-initiator --db_user root --db_password ''
LANG=ja_JP.utf8 LC_ALL=ja_JP.UTF-8 ./client/test/feature/run_test.sh
