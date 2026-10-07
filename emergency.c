#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define PATIENT_PIPE "patient_pipe"
#define HOSPITAL_FIFO "hospital_fifo"
#define LOG_FILE "alerts.log"
#define MSG_SIZE 1024

#define COLOR_RESET  "\033[0m"
#define COLOR_GREEN  "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_RED    "\033[1;31m"


typedef struct {
    int patientId;
    int heartRate;
    int systolicBP;
    int diastolicBP;
    int temperature;
    int oxygenLevel;
} PatientData;


/* Get color according to status */
const char *getStatusColor(const char *status)
{
    if (strcmp(status, "EMERGENCY") == 0)
        return COLOR_RED;

    if (strcmp(status, "WARNING") == 0)
        return COLOR_YELLOW;

    return COLOR_GREEN;
}


/*
 * Health classification.
 *
 * These are simplified thresholds for the academic project.
 */
void getHealthInfo(PatientData data,
                   char *status,
                   char *condition,
                   char *prevention)
{
    /*
     * EMERGENCY
     */
    if (data.heartRate > 120 ||
        data.systolicBP > 140 ||
        data.diastolicBP > 90 ||
        data.temperature > 39 ||
        data.oxygenLevel < 90)
    {
        strcpy(status, "EMERGENCY");

        strcpy(condition,
               "Abnormal vital signs detected");

        strcpy(prevention,
               "Immediate medical attention and doctor notification required.");
    }

    /*
     * WARNING
     */
    else if (data.heartRate < 60 ||
             data.heartRate > 100 ||
             data.systolicBP > 130 ||
             data.diastolicBP > 80 ||
             data.temperature > 37 ||
             data.oxygenLevel < 95)
    {
        strcpy(status, "WARNING");

        strcpy(condition,
               "One or more vital signs are outside the normal range");

        strcpy(prevention,
               "Keep patient under observation and continue monitoring.");
    }

    /*
     * NORMAL
     */
    else
    {
        strcpy(status, "NORMAL");

        strcpy(condition,
               "Vital signs are within the normal range");

        strcpy(prevention,
               "Continue regular monitoring.");
    }
}


/* Save information to alerts.log */
void logAlert(PatientData data,
              const char *status,
              const char *condition,
              const char *prevention)
{
    FILE *log = fopen(LOG_FILE, "a");

    if (log == NULL)
    {
        perror("Could not open alerts.log");
        return;
    }

    time_t now = time(NULL);

    char timeBuffer[64];

    strftime(timeBuffer,
             sizeof(timeBuffer),
             "%Y-%m-%d %H:%M:%S",
             localtime(&now));


    fprintf(log,
            "[%s] Patient ID: %d | "
            "Heart Rate: %d BPM | "
            "BP: %d/%d mmHg | "
            "Temperature: %d C | "
            "Oxygen Level: %d%% | "
            "Status: %s | "
            "Condition: %s | "
            "Action: %s\n",

            timeBuffer,
            data.patientId,
            data.heartRate,
            data.systolicBP,
            data.diastolicBP,
            data.temperature,
            data.oxygenLevel,
            status,
            condition,
            prevention);

    fclose(log);
}


int main()
{
    PatientData data;

    char status[20];
    char condition[150];
    char prevention[250];

    printf("====================================\n");
    printf("       EMERGENCY PROCESSOR\n");
    printf("====================================\n");


    /*
     * Create hospital FIFO.
     */
    if (mkfifo(HOSPITAL_FIFO, 0666) == -1)
    {
        if (errno != EEXIST)
        {
            perror("mkfifo hospital_fifo failed");
            return 1;
        }
    }


    while (1)
    {
        printf("\n[EMERGENCY PROCESSOR]\n");
        printf("Waiting for patient data...\n");


        /*
         * Read from patient_pipe.
         */
        int pipe_fd =
            open(PATIENT_PIPE, O_RDONLY);

        if (pipe_fd == -1)
        {
            perror("Could not open patient_pipe");
            return 1;
        }


        ssize_t bytes =
            read(pipe_fd,
                 &data,
                 sizeof(PatientData));

        close(pipe_fd);


        if (bytes != sizeof(PatientData))
        {
            printf("Invalid patient data received.\n");
            continue;
        }


        /*
         * Display received information.
         */
        printf("\n------------------------------------\n");

        printf("Received Patient ID: %d\n",
               data.patientId);

        printf("Received Heart Rate: %d BPM\n",
               data.heartRate);

        printf("Received BP: %d/%d mmHg\n",
               data.systolicBP,
               data.diastolicBP);

        printf("Received Temperature: %d C\n",
               data.temperature);

        printf("Received Oxygen Level: %d%%\n",
               data.oxygenLevel);


        /*
         * Analyze patient.
         */
        getHealthInfo(data,
                      status,
                      condition,
                      prevention);


        const char *color =
            getStatusColor(status);


        printf("Status: %s%s%s\n",
               color,
               status,
               COLOR_RESET);

        printf("Possible Condition : %s\n",
               condition);

        printf("Recommended Action : %s\n",
               prevention);

        printf("------------------------------------\n");


        /* Save to log */
        logAlert(data,
                 status,
                 condition,
                 prevention);


        /*
         * Prepare FIFO message.
         *
         * Format:
         *
         * ID|HR|SYS|DIA|TEMP|OXYGEN|STATUS|CONDITION|PREVENTION
         */
        char message[MSG_SIZE];

        snprintf(message,
                 sizeof(message),
                 "%d|%d|%d|%d|%d|%d|%s|%s|%s",

                 data.patientId,
                 data.heartRate,
                 data.systolicBP,
                 data.diastolicBP,
                 data.temperature,
                 data.oxygenLevel,

                 status,
                 condition,
                 prevention);


        /*
         * Send to Doctor Alert Process.
         */
        printf("\nSending alert to Doctor Alert Process...\n");

        int fifo_fd =
            open(HOSPITAL_FIFO, O_WRONLY);

        if (fifo_fd == -1)
        {
            perror("Could not open hospital_fifo");
            return 1;
        }


        write(fifo_fd,
              message,
              strlen(message) + 1);

        close(fifo_fd);


        printf("Alert sent successfully to Doctor Alert Process.\n");
    }

    return 0;
}
