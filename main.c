#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define PATIENT_PIPE "patient_pipe"

typedef struct {
    int patientId;
    int heartRate;
    int systolicBP;
    int diastolicBP;
    int temperature;
    int oxygenLevel;
} PatientData;


/* Read integer input */
int readInt(const char *prompt, int min, int max)
{
    int value;

    while (1)
    {
        printf("%s", prompt);

        if (scanf("%d", &value) != 1)
        {
            int c;

            while ((c = getchar()) != '\n' && c != EOF)
            {
            }

            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        if (value < min || value > max)
        {
            printf("Value must be between %d and %d.\n",
                   min, max);
            continue;
        }

        return value;
    }
}


/* Read BP in the form 120/80 */
void readBP(int *systolic, int *diastolic)
{
    char bp[30];

    while (1)
    {
        printf("Enter BP: ");

        scanf("%29s", bp);

        if (sscanf(bp, "%d/%d", systolic, diastolic) == 2)
        {
            if (*systolic >= 50 && *systolic <= 250 &&
                *diastolic >= 30 && *diastolic <= 150)
            {
                return;
            }
        }

        printf("Invalid BP. Please enter like 120/80.\n");
    }
}


int main()
{
    PatientData data;

    printf("====================================\n");
    printf("     PATIENT MONITORING SYSTEM\n");
    printf("====================================\n");

    /*
     * Create the named pipe for communication
     * between Patient Monitoring and Emergency Processor.
     */
    if (mkfifo(PATIENT_PIPE, 0666) == -1)
    {
        if (errno != EEXIST)
        {
            perror("mkfifo failed");
            return 1;
        }
    }


    while (1)
    {
        printf("\n[PATIENT MONITORING PROCESS]\n");

        /* 1. Patient ID */
        data.patientId =
            readInt("Enter Patient ID: ", 1, 999999);

        /* 2. Heart Rate */
        data.heartRate =
            readInt("Enter Heart Rate: ", 1, 300);

        /* 3. Blood Pressure */
        readBP(&data.systolicBP,
               &data.diastolicBP);

        /* 4. Temperature */
        data.temperature =
            readInt("Enter Temperature: ", 30, 45);

        /* 5. Oxygen Level */
        data.oxygenLevel =
            readInt("Enter Oxygen Level: ", 50, 100);


        printf("\n------------------------------------\n");

        printf("Monitoring Patient %d...\n",
               data.patientId);

        printf("Heart Rate    : %d BPM\n",
               data.heartRate);

        printf("BP            : %d/%d mmHg\n",
               data.systolicBP,
               data.diastolicBP);

        printf("Temperature   : %d C\n",
               data.temperature);

        printf("Oxygen Level  : %d%%\n",
               data.oxygenLevel);

        printf("------------------------------------\n");


        /*
         * Open pipe for writing.
         * This sends data to emergency.c.
         */
        printf("Sending patient data to Emergency Processor...\n");

        int fd = open(PATIENT_PIPE, O_WRONLY);

        if (fd == -1)
        {
            perror("Could not open patient_pipe");
            return 1;
        }

        write(fd,
              &data,
              sizeof(PatientData));

        close(fd);

        printf("Patient data sent successfully.\n");


        /* Ask for another patient */
        char choice;

        printf("\nMonitor another patient? (y/n): ");
        scanf(" %c", &choice);

        if (choice != 'y' && choice != 'Y')
        {
            break;
        }
    }


    printf("\nPatient Monitoring Process ended.\n");

    return 0;
}
