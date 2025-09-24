
#include "airline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Globals
Flight flights[MAX_FLIGHTS];
int flightCount = 0;
Passenger passengers[MAX_PASSENGERS];
int passengerCount = 0;
CheckInNode* checkInFront = NULL;
CheckInNode* checkInRear = NULL;

// Functions
void addFlight() {
    if (flightCount >= MAX_FLIGHTS) {
        printf("Flight list full!\n");
        return;
    }
    Flight* f = &flights[flightCount];
    printf("\n=== ADD NEW FLIGHT ===\n");
    printf("Flight Number: ");
    scanf("%s", f->flightNumber);
    printf("Origin: ");
    scanf("%s", f->origin);
    printf("Destination: ");
    scanf("%s", f->destination);
    printf("Departure Time (HH:MM): ");
    scanf("%s", f->departureTime);
    printf("Arrival Time (HH:MM): ");
    scanf("%s", f->arrivalTime);
    printf("Aircraft: ");
    scanf("%s", f->aircraft);
    printf("Capacity: ");
    scanf("%d", &f->capacity);
    printf("Price: $");
    scanf("%f", &f->price);
    printf("Priority (1-10): ");
    scanf("%d", &f->priority);
    f->bookedSeats = 0;
    strcpy(f->status, "scheduled");
    flightCount++;
    printf("Flight added successfully!\n");
}

void displayFlights() {
    printf("\n=== ALL FLIGHTS ===\n");
    printf("%-10s %-12s %-12s %-12s %-10s %-15s\n", "Flight", "Route", "Departure", "Arrival", "Price", "Available");
    for (int i = 0; i < flightCount; i++) {
        Flight* f = &flights[i];
        printf("%-10s %-12s %-12s %-12s $%-9.2f %-15d\n", f->flightNumber, strcat(strcpy((char[32]){}, f->origin), strcat(strcpy((char[32]){}, "-"), f->destination)), f->departureTime, f->arrivalTime, f->price, f->capacity - f->bookedSeats);
    }
}

void bookTicket() {
    if (passengerCount >= MAX_PASSENGERS) {
        printf("Passenger list full!\n");
        return;
    }
    Passenger* p = &passengers[passengerCount];
    printf("\n=== BOOK TICKET ===\n");
    displayFlights();
    printf("Enter passenger details:\n");
    printf("Passenger ID: ");
    scanf("%s", p->id);
    printf("First Name: ");
    scanf("%s", p->firstName);
    printf("Last Name: ");
    scanf("%s", p->lastName);
    printf("Email: ");
    scanf("%s", p->email);
    printf("Phone: ");
    scanf("%s", p->phone);
    printf("Flight Number: ");
    scanf("%s", p->flightId);
    for (int i = 0; i < flightCount; i++) {
        Flight* f = &flights[i];
        if (strcmp(f->flightNumber, p->flightId) == 0) {
            if (f->bookedSeats < f->capacity) {
                f->bookedSeats++;
                strcpy(p->checkInStatus, "pending");
                passengerCount++;
                printf("Ticket booked successfully!\n");
                return;
            } else {
                printf("Flight is full!\n");
                return;
            }
        }
    }
    printf("Flight not found!\n");
}

void displayPassengers() {
    printf("\n=== ALL PASSENGERS ===\n");
    printf("%-8s %-15s %-20s %-12s %-15s\n", "ID", "Name", "Email", "Flight", "Status");
    for (int i = 0; i < passengerCount; i++) {
        Passenger* p = &passengers[i];
        printf("%-8s %-15s %-20s %-12s %-15s\n", p->id, strcat(strcpy((char[32]){}, p->firstName), strcat(strcpy((char[32]){}, " "), p->lastName)), p->email, p->flightId, p->checkInStatus);
    }
}

void enqueueCheckIn(Passenger passenger, int priority) {
    CheckInNode* newNode = (CheckInNode*)malloc(sizeof(CheckInNode));
    newNode->passenger = passenger;
    newNode->priority = priority;
    time_t now = time(0);
    strcpy(newNode->checkInTime, ctime(&now));
    newNode->next = NULL;
    if (!checkInFront) {
        checkInFront = checkInRear = newNode;
    } else if (priority == 1) {
        newNode->next = checkInFront;
        checkInFront = newNode;
    } else {
        checkInRear->next = newNode;
        checkInRear = newNode;
    }
}

void displayCheckInQueue() {
    printf("\n=== CHECK-IN QUEUE ===\n");
    printf("%-15s %-15s %-10s\n", "Passenger", "Flight", "Priority");
    CheckInNode* curr = checkInFront;
    while (curr) {
        printf("%-15s %-15s %-10s\n", strcat(strcpy((char[32]){}, curr->passenger.firstName), strcat(strcpy((char[32]){}, " "), curr->passenger.lastName)), curr->passenger.flightId, curr->priority == 1 ? "VIP" : "Regular");
        curr = curr->next;
    }
}

void checkInPassenger() {
    char passengerId[NAME_LEN];
    int priority;
    printf("\n=== CHECK-IN PASSENGER ===\n");
    printf("Passenger ID: ");
    scanf("%s", passengerId);
    printf("Priority (1=VIP, 2=Regular): ");
    scanf("%d", &priority);
    for (int i = 0; i < passengerCount; i++) {
        if (strcmp(passengers[i].id, passengerId) == 0) {
            enqueueCheckIn(passengers[i], priority);
            printf("Passenger added to check-in queue!\n");
            return;
        }
    }
    printf("Passenger not found!\n");
}

void processCheckInQueue() {
    printf("\n=== PROCESSING CHECK-IN QUEUE ===\n");
    if (!checkInFront) {
        printf("No passengers in queue.\n");
        return;
    }
    CheckInNode* temp = checkInFront;
    Passenger p = temp->passenger;
    printf("Processing check-in for: %s %s\n", p.firstName, p.lastName);
    printf("Flight: %s\n", p.flightId);
    for (int i = 0; i < passengerCount; i++) {
        if (strcmp(passengers[i].id, p.id) == 0) {
            strcpy(passengers[i].checkInStatus, "checked-in");
            break;
        }
    }
    checkInFront = checkInFront->next;
    if (!checkInFront) checkInRear = NULL;
    free(temp);
    printf("Check-in completed successfully!\n");
}

void initializeSampleData() {
    // Sample flights
    Flight f1 = {"AA101", "NYC", "LAX", "08:00", "11:30", "Boeing737", 150, 45.99, 1, 0, "scheduled"};
    Flight f2 = {"UA205", "CHI", "MIA", "14:30", "18:00", "AirbusA320", 180, 289.99, 2, 0, "scheduled"};
    Flight f3 = {"DL308", "ATL", "SEA", "09:15", "12:45", "Boeing757", 200, 320.50, 3, 0, "scheduled"};
    Flight f4 = {"SW412", "DAL", "LAS", "16:20", "17:40", "Boeing737", 143, 199.99, 4, 0, "scheduled"};
    flights[flightCount++] = f1;
    flights[flightCount++] = f2;
    flights[flightCount++] = f3;
    flights[flightCount++] = f4;
    // Sample passengers
    Passenger p1 = {"P001", "John", "Doe", "john@email.com", "555-1234", "AA101", "pending"};
    Passenger p2 = {"P002", "Jane", "Smith", "jane@email.com", "555-5678", "UA205", "pending"};
    Passenger p3 = {"P003", "Bob", "Johnson", "bob@email.com", "555-9012", "DL308", "pending"};
    Passenger p4 = {"P004", "Alice", "Brown", "alice@email.com", "555-3456", "SW412", "pending"};
    passengers[passengerCount++] = p1;
    passengers[passengerCount++] = p2;
    passengers[passengerCount++] = p3;
    passengers[passengerCount++] = p4;
    // Add some passengers to check-in queue
    enqueueCheckIn(p1, 2);
    enqueueCheckIn(p2, 1);
    enqueueCheckIn(p3, 2);
    printf("Sample data initialized successfully!\n");
}

void mainMenu() {
    int choice;
    while (1) {
    printf("\n+--------------------------------------+\n");
    printf("|      AIRLINE MANAGEMENT SYSTEM      |\n");
    printf("+--------------------------------------+\n");
    printf("| 1. Add Flight                       |\n");
    printf("| 2. Book Ticket                      |\n");
    printf("| 3. Check-in Passenger               |\n");
    printf("| 4. Process Check-in Queue           |\n");
    printf("| 5. Display All Flights              |\n");
    printf("| 6. Display All Passengers           |\n");
    printf("| 7. Display Check-in Queue           |\n");
    printf("| 8. Initialize Sample Data           |\n");
    printf("| 0. Exit                             |\n");
    printf("+--------------------------------------+\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: addFlight(); break;
            case 2: bookTicket(); break;
            case 3: checkInPassenger(); break;
            case 4: processCheckInQueue(); break;
            case 5: displayFlights(); break;
            case 6: displayPassengers(); break;
            case 7: displayCheckInQueue(); break;
            case 8: initializeSampleData(); break;
            case 0:
                printf("Thank you for using Airline Management System!\n");
                exit(0);
            default:
                printf("Invalid choice! Please try again.\n");
        }
    }
}
