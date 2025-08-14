//Class created for handling log files
//Simone Valdre', 14/08/2025 (created)

#ifndef _DATALOG
#define _DATALOG

#include <cstdio>
#include <vector>
#include <ctime>

class DataLog {
private:
    FILE *flog;
    int last_day, last_flush;
    int flush_interval;
public:
    //default flush interval: 1h
    DataLog(const int &_flush_interval = 3600);
    ~DataLog();

    int WriteData(const int &timesec, const std::vector < double > &data);
    void Close();
};




#endif