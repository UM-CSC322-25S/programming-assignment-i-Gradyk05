#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_BOATS 120

typedef enum {
    SLIP,           // slip number [1..85]
    LAND,           // bay letter [A..Z]
    TRAILOR,        // trailor license plate
    STORAGE,        // storage shed [1..50]
    LOCATION_UNKNOWN
} BoatLocation;

struct Boat {
    char name[128];
    float size;
    double moneyOwed;
    BoatLocation location;
    union {
        int  slipNumber;
        char landBayLetter;
        char trailorPlate[6];
        int  storageShedNumber;
    } data;
};

struct Boat *marina[MAX_BOATS] = { NULL };

BoatLocation parseLocation(const char *locStr) {
    if (strcmp(locStr, "slip") == 0) {
        return SLIP;
    } else if (strcmp(locStr, "land") == 0) {
        return LAND;
    } else if (strcmp(locStr, "trailor") == 0) {
        return TRAILOR;
    } else if (strcmp(locStr, "storage") == 0) {
        return STORAGE;
    }
    return LOCATION_UNKNOWN;
}

struct Boat *createBoat(const char *name, float size, double moneyOwed, BoatLocation location) {
    struct Boat *b = malloc(sizeof(struct Boat));
    if (!b) {
        fprintf(stderr, "too many boats!!!\n");
        return NULL;
    }
    strncpy(b->name, name, sizeof(b->name) - 1);
    b->name[sizeof(b->name) - 1] = '\0';
    b->size = size;
    b->moneyOwed = moneyOwed;
    b->location = location;
    b->data.slipNumber = 0;  // Default initialization for the union
    return b;
}

int addBoatToMarina(struct Boat *boat) {
    if (!boat) {
        return -1;
    }
    int insertIndex = 0;
    while (insertIndex < MAX_BOATS) {
        if (marina[insertIndex] == NULL) {
            break;
        }
        if (strcmp(boat->name, marina[insertIndex]->name) < 0) {
            break;
        }
        insertIndex++;
    }
    if (marina[MAX_BOATS - 1] != NULL) {
        return -1; // No room left to insert
    }
    for (int j = MAX_BOATS - 1; j > insertIndex; j--) {
        marina[j] = marina[j - 1];
    }
    marina[insertIndex] = boat;
    return insertIndex; 
}

int loadMarinaFromCSV(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "file not found\n");
        return -1;
    }
    char line[256];
    int countLoaded = 0;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        char *parts[5];
        char *token = strtok(line, ",");
        int index = 0;
        while (token && index < 5) {
            parts[index++] = token;
            token = strtok(NULL, ",");
        }
        if (index < 5) {
            fprintf(stderr, "Warning: ignoring invalid line: '%s'\n", line);
            continue;
        }
        char *nameStr     = parts[0];
        char *sizeStr     = parts[1];
        char *locationStr = parts[2];
        char *locDataStr  = parts[3];
        char *moneyStr    = parts[4];
        float sizeVal = strtof(sizeStr, NULL);
        double moneyVal = strtod(moneyStr, NULL);
        BoatLocation loc = parseLocation(locationStr);
        if (loc == LOCATION_UNKNOWN) {
            fprintf(stderr, "Warning: unknown location '%s'. Skipping line.\n", locationStr);
            continue;
        }
        struct Boat *b = createBoat(nameStr, sizeVal, moneyVal, loc);
        if (!b) {
            fprintf(stderr, "Error: boat creation failed.\n");
            continue;
        }
        switch (b->location) {
            case SLIP:
                b->data.slipNumber = atoi(locDataStr);
                break;
            case LAND:
                b->data.landBayLetter = locDataStr[0];
                break;
            case TRAILOR:
                strncpy(b->data.trailorPlate, locDataStr, sizeof(b->data.trailorPlate) - 1);
                b->data.trailorPlate[sizeof(b->data.trailorPlate) - 1] = '\0';
                break;
            case STORAGE:
                b->data.storageShedNumber = atoi(locDataStr);
                break;
            default:
                break;
        }
        int pos = addBoatToMarina(b);
        if (pos == -1) {
            fprintf(stderr, "Marina is full; cannot add more boats.\n");
            free(b);
            continue;
        }
        countLoaded++;
    }
    fclose(fp);
    return countLoaded;
}

const char *locationToString(BoatLocation loc) {
    switch (loc) {
        case SLIP:    return "slip";
        case LAND:    return "land";
        case TRAILOR: return "trailor";
        case STORAGE: return "storage";
        default:      return "unknown";
    }
}

int saveMarinaToCSV(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: cannot open file '%s' for writing.\n", filename);
        return -1;
    }
    for (int i = 0; i < MAX_BOATS; i++) {
        struct Boat *b = marina[i];
        if (b != NULL) {
            const char *locStr = locationToString(b->location);
            fprintf(fp, "%s,%.2f,%s,", b->name, b->size, locStr);
            switch (b->location) {
                case SLIP:
                    fprintf(fp, "%d,", b->data.slipNumber);
                    break;
                case LAND:
                    fprintf(fp, "%c,", b->data.landBayLetter);
                    break;
                case TRAILOR:
                    fprintf(fp, "%s,", b->data.trailorPlate);
                    break;
                case STORAGE:
                    fprintf(fp, "%d,", b->data.storageShedNumber);
                    break;
                default:
                    fprintf(fp, "?,");
                    break;
            }
            fprintf(fp, "%.2f\n", b->moneyOwed);
        }
    }
    fclose(fp);
    return 0;
}

void removeBoatFromMarina(int index) {
    if (index < 0 || index >= MAX_BOATS) {
        fprintf(stderr, "No boat with that name\n");
        return;
    }
    if (marina[index] == NULL) {
        fprintf(stderr, "Warning: no boat at index %d.\n", index);
        return;
    }
    free(marina[index]);
    marina[index] = NULL;
    for (int i = index; i < MAX_BOATS - 1; i++) {
        marina[i] = marina[i + 1];
    }
    marina[MAX_BOATS - 1] = NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <BoatData.csv>\n", argv[0]);
        return 1;
    }
    const char *csvFile = argv[1];
    int loaded = loadMarinaFromCSV(csvFile);
    printf("Loaded %d boats from '%s'\n", loaded, csvFile);
    char command;
    bool running = true;
    while (running) {
        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it :\n");
        int result = scanf(" %c", &command);
        if (result == EOF) {
            break;
        }
        command = tolower((unsigned char)command);
        switch (command) {
            case 'a': {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF);
                char inputLine[256];
                printf("\nPlease enter the boat data in CSV format : ");
                if (fgets(inputLine, sizeof(inputLine), stdin) == NULL)
                    break;
                inputLine[strcspn(inputLine, "\n")] = '\0';
                char *nameStr = strtok(inputLine, ",");
                char *sizeStr = strtok(NULL, ",");
                char *locationStr = strtok(NULL, ",");
                char *locDataStr = strtok(NULL, ",");
                char *moneyStr = strtok(NULL, ",");
                if (!nameStr || !sizeStr || !locationStr || !locDataStr || !moneyStr) {
                    printf("Invalid input format.\n");
                    break;
                }
                float size = strtof(sizeStr, NULL);
                double moneyOwed = strtod(moneyStr, NULL);
                BoatLocation loc = parseLocation(locationStr);
                if (loc == LOCATION_UNKNOWN) {
                    printf("Unknown location: %s\n", locationStr);
                    break;
                }
                struct Boat *b = createBoat(nameStr, size, moneyOwed, loc);
                if (!b) {
                    printf("Boat creation failed.\n");
                    break;
                }
                switch (loc) {
                    case SLIP:
                        b->data.slipNumber = atoi(locDataStr);
                        break;
                    case LAND:
                        b->data.landBayLetter = locDataStr[0];
                        break;
                    case TRAILOR:
                        strncpy(b->data.trailorPlate, locDataStr, sizeof(b->data.trailorPlate) - 1);
                        b->data.trailorPlate[sizeof(b->data.trailorPlate) - 1] = '\0';
                        break;
                    case STORAGE:
                        b->data.storageShedNumber = atoi(locDataStr);
                        break;
                    default:
                        break;
                }
                int pos = addBoatToMarina(b);
                if (pos == -1) {
                    printf("Marina is full. Boat not added.\n");
                    free(b);
                } 
                break;
            }
            case 'i': {
                for (int i = 0; i < MAX_BOATS; i++) {
                    if (marina[i] != NULL) {
                        printf("%-20s %2.0f'    ", marina[i]->name, marina[i]->size);
                        switch (marina[i]->location) {
                            case SLIP:
                                printf("slip   # %d", marina[i]->data.slipNumber);
                                break;
                            case LAND:
                                printf("land %c", marina[i]->data.landBayLetter);
                                break;
                            case TRAILOR:
                                printf("trailor %s", marina[i]->data.trailorPlate);
                                break;
                            case STORAGE:
                                printf("storage # %d", marina[i]->data.storageShedNumber);
                                break;
                            default:
                                printf("unknown location");
                                break;
                        }
                        printf("   Owes $%.2f\n", marina[i]->moneyOwed);
                    }
                }
                break;
            }
            case 'p': {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF);
                char boatName[128];
                printf("\nPlease enter the boat name for payment: ");
                if (fgets(boatName, sizeof(boatName), stdin) != NULL) {
                    boatName[strcspn(boatName, "\n")] = '\0';
                    int boatIndex = -1;
                    for (int i = 0; i < MAX_BOATS; i++) {
                        if (marina[i] != NULL && strcasecmp(marina[i]->name, boatName) == 0) {
                            boatIndex = i;
                            break;
                        }
                    }
                    if (boatIndex == -1) {
                        printf("Boat not found.\n");
                    } else {
                        char input[100];
                        printf("Please enter the amount to be paid: ");
                        if (fgets(input, sizeof(input), stdin) != NULL) {
                            double paid = strtod(input, NULL);
                            if (marina[boatIndex]->moneyOwed < paid) {
                                fprintf(stderr, "That is more than the amount owed, $%.2f\n", marina[boatIndex]->moneyOwed);
                            } else {
                                marina[boatIndex]->moneyOwed -= paid;
                                printf("Payment processed. New amount owed: $%.2f\n", marina[boatIndex]->moneyOwed);
                            }
                        }
                    }
                }
                break;
            }
            case 'm': {
                for (int i = 0; i < MAX_BOATS; i++) {
                    if (marina[i] != NULL) {
                        switch (marina[i]->location) {
                            case SLIP:
                                marina[i]->moneyOwed += (12.5 * marina[i]->size);
                                break;
                            case LAND:
                                marina[i]->moneyOwed += (14 * marina[i]->size);
                                break;
                            case TRAILOR:
                                marina[i]->moneyOwed += (25 * marina[i]->size);
                                break;
                            case STORAGE:
                                marina[i]->moneyOwed += (11.2 * marina[i]->size);
                                break;
                            default:
                                printf("The boat's location is unknown.\n");
                                break;
                        }
                    }
                }
                break;
            }
            case 'r': {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF);  // Clear any leftover input

                char boatName[128];
                printf("\nPlease enter the boat name for removal: ");
                if (fgets(boatName, sizeof(boatName), stdin) != NULL) {
                    boatName[strcspn(boatName, "\n")] = '\0';  // Remove trailing newline

                    int boatIndex = -1;
                    for (int i = 0; i < MAX_BOATS; i++) {
                        if (marina[i] != NULL && strcasecmp(marina[i]->name, boatName) == 0) {
                            boatIndex = i;
                            break;
                        }
                    }
                    if (boatIndex == -1) {
                        printf("Boat not found.\n");
                    } else {
                        removeBoatFromMarina(boatIndex);
                        printf("Boat removed.\n");
                    }
                }
                break;
            }

            case 'x': {
                printf("Exiting program.\n");
                running = false;
                break;
            }
            default:
                printf("Unrecognized command '%c'. Please try again.\n", command);
                break;
        }
    }
    if (saveMarinaToCSV(csvFile) == 0) {
        printf("Saved boats to '%s'\n", csvFile);
    }
    for (int i = 0; i < MAX_BOATS; i++) {
        if (marina[i]) {
            free(marina[i]);
            marina[i] = NULL;
        }
    }
    return 0;
}




