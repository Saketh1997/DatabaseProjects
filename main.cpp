/*
Skeleton code for storage and buffer management
*/

#include <string>
#include <ios>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "classes.h"
using namespace std;


int main() {
    StorageManager manager("EmployeeRelation.dat");

    cout << "Loading records from Employee.csv into EmployeeRelation.dat..." << endl;
    createFromFile("Employee.csv", manager);
    int id = 1;

    while(id != 0){
        cout << "Enter the Employee ID to search for or '0' if you want to quit: ";
        cin >> id;
        if (id == 0){
            return 0
        }else{
            manager.findRecordById("EmployeeRelation.dat", id);
        }
        
    }
    return 0;
}