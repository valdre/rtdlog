#include "datalog.h"



DataLog::DataLog(const int &_flush_interval) {
    flog = NULL;
    last_day = -1; last_flush = -1;
    flush_interval = _flush_interval;
}

DataLog::~DataLog() {
    if(flog) fclose(flog);
}

void DataLog::Close() {
    if(flog == NULL) return;
    fclose(flog);
    flog = NULL;
    return;
}

int DataLog::WriteData(const int &timesec, const std::vector < double > &data) {
    bool do_flush = false;
    if(last_flush < 0) last_flush = timesec;
    else if(timesec - last_flush >= flush_interval) {
        do_flush = true;
        last_flush = timesec;
    }

    time_t timep = timesec;
    struct tm *tloc = localtime(&timep);
    int today = (tloc->tm_year + 1900) * 10000 + (tloc->tm_mon+1) * 100 + tloc->tm_mday;
    if(last_day < 0) last_day = today;

    if(flog && last_day != today) {
        fclose(flog);
        flog = NULL;
    }


    if(flog == NULL) {
        printf("DEBUG: opening a new log file!\n\n");
        last_day = today;
        
        char fname[100];
        sprintf(fname, "db/%08d.log", today);
        flog = fopen(fname, "a");
        if(flog == NULL) {
            perror(fname);
            return -1;
        }
    }

    fprintf(flog, "%02d:%02d", tloc->tm_hour, tloc->tm_min);
    for(size_t i = 0; i < data.size(); i++) {
        fprintf(flog, " % 6.3f", data[i]);
    }
    fprintf(flog, "\n");

    if(do_flush) {
        if(fflush(flog)) perror("fflush");
    }
    return 0;
}
