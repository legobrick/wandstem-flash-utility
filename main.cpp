/***************************************************************************
 *   Copyright (C) 2017 by Paolo Polidori                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/
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