#include <iostream>
#include "Program.h"
#include "Exceptions.h"

using namespace std;

int main(int argc, const char *argv[]) {

    cout << "Welcome to the Wandstem device utility!" << endl << endl;
    Program &p = Program::get_instance();
    try {
        p.init(argc, argv);
    } catch (WontExecuteException &ex) {
        //asked for help
        return 1;
    } catch (DeviceNotFoundException &ex) {
        cout << ex.what();
        return 1;
    }
    p.flash_if_needed();
    p.read_to_end();
}