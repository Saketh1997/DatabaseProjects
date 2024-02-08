#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <limits>

using namespace std;

class Record {
public:
    int id, manager_id;
    string name, bio;

    Record() : id(0), manager_id(0) {}

    Record(const vector<string>& fields) {
        if (!fields.empty()) {
            id = stoi(fields[0]);
            name = fields[1];
            bio = fields[2];
            manager_id = stoi(fields[3]);
        }
    }

    string serialize() const {
        stringstream ss;
        // Serialize with length information for variable-length fields
        ss << id << "," << name.length() << "," << name << "," << bio.length() << "," << bio << "," << manager_id;
        return ss.str();
    }

    void print() const {
        cout << "ID: " << id << ", Name: " << name << ", Bio: " << bio << ", Manager ID: " << manager_id << endl;
    }

    static Record deserialize(const string& str) {
        stringstream ss(str);
        string part;
        vector<string> parts;

        while (getline(ss, part, ',')) {
            parts.push_back(part);
        }

        if (parts.size() < 6) {
            return Record(); // Ensure we have enough parts for id, name length, name, bio length, bio, manager_id
        }

        // Adjust for correct extraction of name and bio with their lengths
        int nameLength = stoi(parts[1]);
        string name = parts[2].substr(0, nameLength); // Extract name using its length
        int bioLength = stoi(parts[3]);
        string bio = parts[4].substr(0, bioLength); // Extract bio using its length

        // Correct the fields vector for the constructor
        vector<string> fields = {parts[0], name, bio, parts[5]};

        return Record(fields);
    }

    bool isValid() const {
        return id != 0;
    }
};

class Page {
public:
    struct RecordInfo {
        int id;
        int offset;
        int length;
    };

    vector<RecordInfo> directory;
    string data;
    static const int MaxSize = 4096; // Page size limit

    bool canAddRecord(const Record& record) const {
        string serialized = record.serialize();
        // Account for directory entry; id (4 bytes) + offset (4 bytes) + length (4 bytes) = 12 bytes
        int requiredSpace = serialized.length() + 12; 
        return (data.size() + requiredSpace <= MaxSize);
    }

    void addRecord(const Record& record, int& currentOffset) {
        string serialized = record.serialize();
        int recordLength = serialized.length();
        directory.push_back({record.id, currentOffset, recordLength});
        data += serialized;
        currentOffset += recordLength;
    }

    string serializeDirectory() const {
        stringstream ss;
        for (const auto& info : directory) {
            ss << info.id << " " << info.offset << " " << info.length << ";";
        }
        return ss.str();
    }
};

class StorageManager {
private:
    vector<Page> pages;
    string fileName;

    void flushPagesToDisk() {
        ofstream outFile(fileName, ios::binary | ios::app);
        for (const auto& page : pages) {
            // Write directory information at the beginning of each page
            string dirSerialized = page.serializeDirectory();
            outFile << dirSerialized << endl; // End directory with a newline for simplicity

            // Write the page data
            outFile.write(page.data.c_str(), page.data.size());
            outFile << endl; // Separate pages with a newline
        }
        pages.clear();
    }

public:
    StorageManager(const string& fName) : fileName(fName) {
        ofstream outFile(fName, ios::binary); // Initialize the file
        if (!outFile) {
            throw runtime_error("Could not open file for initialization.");
        }
    }

    void insertRecord(const Record& record) {
        static int currentOffset = 0;
        if (pages.empty() || !pages.back().canAddRecord(record)) {
            flushPagesToDisk(); // Flush existing pages if they cannot hold the new record
            pages.push_back(Page());
            currentOffset = 0; // Reset offset for new page
        }
        pages.back().addRecord(record, currentOffset);
    }

    void finalize() {
        flushPagesToDisk(); // Ensure all data is written to disk
    }

    void findRecordById(const string& fileName, int searchId) {
        ifstream inFile(fileName, ios::binary);
        if (!inFile) {
            cerr << "Failed to open data file for reading." << endl;
            return;
        }

        string line;
        while (getline(inFile, line)) { // Assuming directory info is line-by-line
            stringstream lineStream(line);
            string item;
            while (getline(lineStream, item, ';')) {
                stringstream itemStream(item);
                int id, offset, length;
                itemStream >> id >> offset >> length;

                if (id == searchId) {
                    inFile.seekg(offset, ios::cur); // Move to the start of the record
                    string recordData(length, '\0');
                    inFile.read(&recordData[0], length);
                    Record foundRecord = Record::deserialize(recordData);
                    if (foundRecord.id == searchId) {
                        foundRecord.print();
                        return;
                    }
                }
            }
            // Skip to the next directory line or page
            inFile.ignore(numeric_limits<streamsize>::max(), '\n');
        }

        cout << "Record with ID " << searchId << " not found." << endl;
    }
};

void createFromFile(const string& csvFileName, StorageManager& manager) {
    ifstream inputFile(csvFileName);
    if (!inputFile.is_open()) {
        throw runtime_error("Could not open CSV file.");
    }

    string line;
    while (getline(inputFile, line)) {
        stringstream ss(line);
        vector<string> fields;
        string field;
        while (getline(ss, field, ',')) {
            fields.push_back(field);
        }
        Record record(fields);
        if (record.isValid()) manager.insertRecord(record);
    }
    manager.finalize();
}