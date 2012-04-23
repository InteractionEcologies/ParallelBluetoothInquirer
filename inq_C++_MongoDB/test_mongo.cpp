#include <iostream>
#include <mongo/client/dbclient.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

using namespace std;

void run() {
  mongo::DBClientConnection c;
  c.connect("localhost");
}


int main(int argc, char* argv[]) {
  try {
    run();
    cout << "connected ok" << endl;
  } catch( mongo::DBException &e ) {
    cout << "caught " << e.what() << endl;
  }
  return 0;
}
