#include "maxlib.h"
#include <cstring>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

int go = 1;
void sigh(int signum) {
    signum++;
    go = 0;
}

typedef struct {
    int n;
    char com[100];
    char arg[50][100];
} frase;

void leggiriga(char *striga,frase *riga) {
    int i,w=0,st=0;
    riga->n=0;
    for(i=0;i<(int)strlen(striga);i++) {
        if(striga[i]>32&&striga[i]<127) {
            if(w==0) {w=1; st=i;}
            if(riga->n==0) riga->com[i-st]=striga[i];
            else riga->arg[riga->n-1][i-st]=striga[i];
        }
        else {
            if(w==1) {
                w=0;
                if(riga->n==0) riga->com[i-st]='\0';
                else riga->arg[riga->n-1][i-st]='\0';
                riga->n++;
            }
        }
    }
    if(w==1) {
        if(riga->n==0) riga->com[i-st]='\0';
        else riga->arg[riga->n-1][i-st]='\0';
        riga->n++;
    }
    riga->n--;
    return;
}

void print_date(const unsigned &sec) {
    time_t timep = sec;
    struct tm *tloc = localtime(&timep);
    printf("%04d-%02d-%02d %02d:%02d:%02d", tloc->tm_year + 1900, tloc->tm_mon+1, tloc->tm_mday, tloc->tm_hour, tloc->tm_min, tloc->tm_sec);
}

int main(int argc, char *argv[]) {
    FILE *fin = NULL;
    if(argc > 1) {
        fin = fopen(argv[1], "r");
        if(fin == NULL) perror(argv[1]);
    }
    if(fin == NULL) {
        printf("Usage: logger <config file>\n");
        printf("       further details in the example config files (cfg/example.cfg)\n");
        printf("\n");
        return 0;
    }

    int dev = 0, ce = 0;
    uint8_t mode = SPI_MODE_1;
    bool wire3 = true, hz50 = true;
    double Rcal = 1.22e-4, Rtmp;

    std::vector < MaxRTDProbe > probes;

    char striga[1000];
    frase riga;

    for(;fgets(striga, 1000, fin)!=NULL;) {
        leggiriga(striga, &riga);
        if(riga.n < 0) continue;
        if((strcmp(riga.com, "spidev") == 0) && (riga.n > 1)) {
            dev = atoi(riga.arg[0]);
            ce = atoi(riga.arg[1]);
            if(riga.n > 2) {
                switch(atoi(riga.arg[2])) {
                    case 0: mode = SPI_MODE_0; break;
                    case 1: mode = SPI_MODE_1; break;
                    case 2: mode = SPI_MODE_2; break;
                    case 3: mode = SPI_MODE_3; break;
                    default:
                    mode = SPI_MODE_1;
                    printf(YEL "Invalid SPI mode (0, 1, 2, or 3 allowed). Reverting to mode 1.\n");
                }
            }
        }
        if((strcmp(riga.com,"wires")==0)&&(riga.n>0)) {
            switch(atoi(riga.arg[0])) {
                case 2: wire3 = false; break;
                case 3: wire3 = true; break;
                case 4: wire3 = false; break;
                default:
                wire3 = false;
                printf(YEL "Invalid wires setting (2, 3, or 4 allowed). Reverting to 2/4 wires.\n");
            }
        }
        if((strcmp(riga.com,"filter")==0)&&(riga.n>0)) {
            switch(atoi(riga.arg[0])) {
                case 60: hz50 = false; break;
                case 50: hz50 = true; break;
                default:
                hz50 = true;
                printf(YEL "Invalid filter setting (50 or 60 allowed). Reverting to 50 Hz filtering.\n");
            }
        }
        if((strcmp(riga.com,"rcal")==0)&&(riga.n>0)) {
            Rcal = atof(riga.arg[0]);
            if(riga.n > 1) {
                Rtmp = atof(riga.arg[1]);
                Rcal /= Rtmp * 32768.;
            }
        }
        
        if(strcmp(riga.com, "add") == 0) {
            MaxRTDProbe probe(dev, ce, mode);
            if(!(probe.IsZombie())) {
                probe.Configure(true, true, wire3, hz50);
                probe.SetRcal(Rcal);
                probes.push_back(probe);
            }
        }
    }
    fclose(fin);

    int stat, Nav = 0;
    std::vector < double > ave(probes.size(), 0);
    double sam;
    struct timeval tnow;
    unsigned tave;
    gettimeofday(&tnow, NULL);
    tave = (1 + tnow.tv_sec / 60) * 60;
    
    signal(SIGINT, sigh);
    while(go) {
        if(tnow.tv_sec >= tave) {
            printf(UP);
            print_date(tnow.tv_sec); printf(":                                 \n");
            if(Nav > 5) {
                for(size_t i = 0; i < probes.size(); i++) {
                    ave[i] /= (double)Nav;
                    printf("    T = %7.3f C  (ADCav = %5.0f)\n", probes[i].Tconv(ave[i]), ave[i]);
                    ave[i] = 0;
                }
                Nav = 0;
            }
            else {
                for(size_t i = 0; i < probes.size(); i++) printf("    not enough samples                    \n");
            }
            printf("\n");
            tave += 60;
        }
        gettimeofday(&tnow, NULL);
        usleep(1000000 - tnow.tv_usec);
        gettimeofday(&tnow, NULL);
        printf(UP);
        print_date(tnow.tv_sec); printf(": ADC readings ->");
        
        for(size_t i = 0; i < probes.size() + 1; i++) {
            stat = probes[i].ADCRead(sam);
            if(stat < 0) goto err_exit;
            ave[i] += sam;
            printf(" %5.0f", sam);
        }
        Nav++;
        printf("\n");
    }
    
    return 0;
    err_exit:
    printf("One or more errors occurred. Exiting...\n");
    return 0;
}
