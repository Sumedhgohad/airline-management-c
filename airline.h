
#ifndef AIRLINE_H
#define AIRLINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_FLIGHTS 100
#define MAX_PASSENGERS 200
#define NAME_LEN 32
#define EMAIL_LEN 64
#define PHONE_LEN 16
#define FLIGHT_ID_LEN 8
#define STATUS_LEN 16

// Flight structure
typedef struct {
    char flightNumber[FLIGHT_ID_LEN];
    char origin[NAME_LEN];
    char destination[NAME_LEN];
    char departureTime[8];
    char arrivalTime[8];
    char aircraft[NAME_LEN];
    int capacity;
    float price;
    int priority;
    int bookedSeats;
    char status[STATUS_LEN];
} Flight;

// Passenger structure
typedef struct {
    char id[NAME_LEN];
    char firstName[NAME_LEN];
    char lastName[NAME_LEN];
    char email[EMAIL_LEN];
    char phone[PHONE_LEN];
    char flightId[FLIGHT_ID_LEN];
    char checkInStatus[STATUS_LEN];
} Passenger;

// Linked List Node for Check-in Queue
typedef struct CheckInNode {
    Passenger passenger;
    int priority;
    char checkInTime[32];
    struct CheckInNode* next;
} CheckInNode;

// Function prototypes
void addFlight();
void displayFlights();
void bookTicket();
void displayPassengers();
void enqueueCheckIn(Passenger passenger, int priority);
void displayCheckInQueue();
void checkInPassenger();
void processCheckInQueue();
void initializeSampleData();
void mainMenu();
void analyzeRouteNetwork();

#endif // AIRLINE_H