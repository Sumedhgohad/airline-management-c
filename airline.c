
#include "airline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <float.h>

// Globals
Flight flights[MAX_FLIGHTS];
int flightCount = 0;
Passenger passengers[MAX_PASSENGERS];
int passengerCount = 0;
CheckInNode* checkInFront = NULL;
CheckInNode* checkInRear = NULL;

#define MAX_ANALYSIS_AIRPORTS 40
#define INF_WEIGHT 1e12
#define APSP_FLOYD_THRESHOLD 10

typedef struct {
    int src;
    int dest;
    double weight;
} RouteEdge;

typedef struct {
    int vertexCount;
    int directed; // bool, but keep int for scanf compatibility
    double **adjMatrix;
} RouteGraph;

typedef struct {
    int src;
    int dest;
    double weight;
} MSTResultEdge;

typedef struct {
    const char* src;
    const char* dest;
    double weight;
} SampleEdge;

typedef struct {
    const char* name;
    const char* description;
    int airportCount;
    int directed;
    const char* const* airportNames;
    const SampleEdge* edges;
    int edgeCount;
} SampleScenario;

static const char* const SAMPLE_DENSE_NAMES[] = {"DEL", "BOM", "BLR", "HYD", "CCU", "PNQ"};
static const SampleEdge SAMPLE_DENSE_EDGES[] = {
    {"DEL", "BOM", 720}, {"DEL", "BLR", 1740}, {"DEL", "HYD", 1265}, {"DEL", "CCU", 1300}, {"DEL", "PNQ", 1440},
    {"BOM", "BLR", 984}, {"BOM", "HYD", 710}, {"BOM", "CCU", 1650}, {"BOM", "PNQ", 148},
    {"BLR", "HYD", 550}, {"BLR", "CCU", 1515}, {"BLR", "PNQ", 840},
    {"HYD", "CCU", 1180}, {"HYD", "PNQ", 620},
    {"CCU", "PNQ", 1655}
};

static const char* const SAMPLE_SPARSE_NAMES[] = {"SEA", "SFO", "LAX", "DEN", "ORD", "JFK", "ATL", "MIA"};
static const SampleEdge SAMPLE_SPARSE_EDGES[] = {
    {"SEA", "SFO", 679},
    {"SFO", "LAX", 337},
    {"LAX", "DEN", 1383},
    {"DEN", "ORD", 1610},
    {"ORD", "JFK", 1188},
    {"JFK", "ATL", 760},
    {"ATL", "MIA", 595}
};

static const char* const SAMPLE_SMALL_APSP_NAMES[] = {"BOS", "PHL", "DCA", "RDU"};
static const SampleEdge SAMPLE_SMALL_APSP_EDGES[] = {
    {"BOS", "PHL", 450},
    {"PHL", "DCA", 225},
    {"DCA", "RDU", 250},
    {"RDU", "BOS", 615},
    {"BOS", "DCA", 400},
    {"PHL", "RDU", 500}
};

static const char* const SAMPLE_LARGE_APSP_NAMES[] = {
    "LHR", "CDG", "FRA", "AMS", "MAD", "BCN", "FCO", "ZRH", "ARN", "IST", "ATH", "DUB", "CPH", "HEL", "OSL", "BRU"
};
static const SampleEdge SAMPLE_LARGE_APSP_EDGES[] = {
    {"LHR", "CDG", 343}, {"LHR", "FRA", 655}, {"LHR", "AMS", 356}, {"LHR", "DUB", 449},
    {"CDG", "FRA", 280}, {"CDG", "BCN", 515}, {"CDG", "BRU", 162},
    {"FRA", "AMS", 365}, {"FRA", "ZRH", 305}, {"FRA", "BRU", 318},
    {"AMS", "CPH", 620}, {"AMS", "BRU", 173}, {"AMS", "ARN", 721},
    {"MAD", "BCN", 505}, {"MAD", "DUB", 1450}, {"MAD", "FCO", 1365},
    {"BCN", "FCO", 855}, {"BCN", "IST", 2230},
    {"FCO", "ATH", 653}, {"FCO", "IST", 1375},
    {"ZRH", "IST", 1760}, {"ZRH", "CPH", 960},
    {"ARN", "HEL", 397}, {"ARN", "OSL", 416},
    {"IST", "ATH", 561}, {"IST", "HEL", 2143},
    {"DUB", "BRU", 780},
    {"CPH", "HEL", 782}, {"CPH", "OSL", 483},
    {"HEL", "OSL", 785}
};

static const SampleScenario SAMPLE_SCENARIOS[] = {
    {
        "Dense hub network",
        "High-frequency routes between the busiest Indian metros. Almost every city connects to every other, ideal for demonstrating Prim's MST on dense graphs.",
        (int)(sizeof(SAMPLE_DENSE_NAMES) / sizeof(SAMPLE_DENSE_NAMES[0])),
        0,
        SAMPLE_DENSE_NAMES,
        SAMPLE_DENSE_EDGES,
        (int)(sizeof(SAMPLE_DENSE_EDGES) / sizeof(SAMPLE_DENSE_EDGES[0]))
    },
    {
        "Sparse coast-to-coast corridor",
        "A simple transcontinental chain of U.S. airports with just enough links to stay connected. Great for showcasing how Kruskal excels on sparse networks.",
        (int)(sizeof(SAMPLE_SPARSE_NAMES) / sizeof(SAMPLE_SPARSE_NAMES[0])),
        0,
        SAMPLE_SPARSE_NAMES,
        SAMPLE_SPARSE_EDGES,
        (int)(sizeof(SAMPLE_SPARSE_EDGES) / sizeof(SAMPLE_SPARSE_EDGES[0]))
    },
    {
        "Regional shuttle circuit",
        "A four-airport directed loop along the U.S. east coast, perfect for demonstrating Floyd-Warshall on a compact network.",
        (int)(sizeof(SAMPLE_SMALL_APSP_NAMES) / sizeof(SAMPLE_SMALL_APSP_NAMES[0])),
        1,
        SAMPLE_SMALL_APSP_NAMES,
        SAMPLE_SMALL_APSP_EDGES,
        (int)(sizeof(SAMPLE_SMALL_APSP_EDGES) / sizeof(SAMPLE_SMALL_APSP_EDGES[0]))
    },
    {
        "Pan-European network",
        "Sixteen major European hubs with many intercontinental legs. The graph is large enough that Johnson + Dijkstra becomes the recommended APSP strategy.",
        (int)(sizeof(SAMPLE_LARGE_APSP_NAMES) / sizeof(SAMPLE_LARGE_APSP_NAMES[0])),
        0,
        SAMPLE_LARGE_APSP_NAMES,
        SAMPLE_LARGE_APSP_EDGES,
        (int)(sizeof(SAMPLE_LARGE_APSP_EDGES) / sizeof(SAMPLE_LARGE_APSP_EDGES[0]))
    }
};

static RouteGraph createRouteGraph(int vertices, int directed);
static void freeRouteGraph(RouteGraph* graph);
static double** allocateMatrix(int n, double initial);
static void freeMatrix(double** matrix, int n);
static void printGraphDiagram(const RouteGraph* graph, char airportNames[][NAME_LEN]);
static int findAirportIndex(char airportNames[][NAME_LEN], int count, const char* name);
static double primMST(const RouteGraph* graph, MSTResultEdge* output, int* edgeCount);
static double kruskalMST(const RouteGraph* graph, RouteEdge* edges, int edgeCount, MSTResultEdge* output, int* mstEdgeCount);
static int floydWarshallAllPairs(const RouteGraph* graph, double** distOut);
static int johnsonAllPairs(const RouteGraph* graph, double** distOut);
static void printDistanceMatrix(double** dist, int n, char airportNames[][NAME_LEN]);
static int buildUndirectedEdgeList(const RouteGraph* graph, RouteEdge** edgesOut);
static void dijkstraReweighted(const RouteGraph* graph, double** reweighted, int src, double* dist);
static int disjointSetFind(int* parent, int v);
static void disjointSetUnion(int* parent, int* rank, int a, int b);
static int compareRouteEdges(const void* a, const void* b);
static const SampleScenario* getSampleScenario(int option);
static int populateSampleScenario(int option, RouteGraph* graph, char airportNames[][NAME_LEN], int* acceptedRoutes, const SampleScenario** scenarioOut);

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
    printf("| 9. Analyze Route Network            |\n");
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
            case 9: analyzeRouteNetwork(); break;
            case 0:
                printf("Thank you for using Airline Management System!\n");
                exit(0);
            default:
                printf("Invalid choice! Please try again.\n");
        }
    }
}

static const SampleScenario* getSampleScenario(int option) {
    int scenarioCount = (int)(sizeof(SAMPLE_SCENARIOS) / sizeof(SAMPLE_SCENARIOS[0]));
    if (option < 1 || option > scenarioCount) {
        return NULL;
    }
    return &SAMPLE_SCENARIOS[option - 1];
}

static int populateSampleScenario(int option, RouteGraph* graph, char airportNames[][NAME_LEN], int* acceptedRoutes, const SampleScenario** scenarioOut) {
    const SampleScenario* scenario = getSampleScenario(option);
    if (!scenario || !graph || !airportNames || !acceptedRoutes) {
        return 0;
    }
    *graph = createRouteGraph(scenario->airportCount, scenario->directed);
    if (!graph->adjMatrix) {
        return 0;
    }
    for (int i = 0; i < scenario->airportCount; ++i) {
        strncpy(airportNames[i], scenario->airportNames[i], NAME_LEN - 1);
        airportNames[i][NAME_LEN - 1] = '\0';
    }
    for (int i = scenario->airportCount; i < MAX_ANALYSIS_AIRPORTS; ++i) {
        airportNames[i][0] = '\0';
    }

    for (int i = 0; i < scenario->edgeCount; ++i) {
        int srcIndex = findAirportIndex(airportNames, scenario->airportCount, scenario->edges[i].src);
        int destIndex = findAirportIndex(airportNames, scenario->airportCount, scenario->edges[i].dest);
        if (srcIndex == -1 || destIndex == -1) {
            continue;
        }
        double weight = scenario->edges[i].weight;
        if (graph->adjMatrix[srcIndex][destIndex] > weight) {
            graph->adjMatrix[srcIndex][destIndex] = weight;
        }
        if (!graph->directed && graph->adjMatrix[destIndex][srcIndex] > weight) {
            graph->adjMatrix[destIndex][srcIndex] = weight;
        }
    }

    *acceptedRoutes = scenario->edgeCount;
    if (scenarioOut) {
        *scenarioOut = scenario;
    }
    return scenario->airportCount;
}

void analyzeRouteNetwork() {
    printf("\n=== ROUTE NETWORK ANALYSIS ===\n");
    printf("Choose input mode:\n");
    printf("  1. Demo dataset - Dense hub network (Prims showcase)\n");
    printf("  2. Demo dataset - Sparse coast-to-coast corridor (Kruskal showcase)\n");
    printf("  3. Demo dataset - Regional shuttle circuit (Floyd-Warshall showcase)\n");
    printf("  4. Demo dataset - Pan-European network (Johnson showcase)\n");
    printf("  5. Manual entry\n");
    printf("Enter choice: ");

    int mode;
    if (scanf("%d", &mode) != 1) {
        printf("Invalid selection. Returning to main menu.\n");
        return;
    }

    RouteGraph graph;
    graph.vertexCount = 0;
    graph.directed = 0;
    graph.adjMatrix = NULL;

    int airportCount = 0;
    int acceptedRoutes = 0;
    char airportNames[MAX_ANALYSIS_AIRPORTS][NAME_LEN];
    const SampleScenario* scenarioInfo = NULL;

    if (mode >= 1 && mode <= 4) {
        airportCount = populateSampleScenario(mode, &graph, airportNames, &acceptedRoutes, &scenarioInfo);
        if (airportCount == 0) {
            printf("Unable to load the requested sample dataset.\n");
            freeRouteGraph(&graph);
            return;
        }
        printf("\nLoaded sample dataset: %s\n", scenarioInfo->name);
        printf("%s\n", scenarioInfo->description);
        printf("Airports: %d  Routes: %d  Directed: %s\n", airportCount, acceptedRoutes, scenarioInfo->directed ? "Yes" : "No");
    } else if (mode == 5) {
        printf("Enter number of airports (2-%d): ", MAX_ANALYSIS_AIRPORTS);
        if (scanf("%d", &airportCount) != 1) {
            printf("Invalid input detected. Returning to main menu.\n");
            return;
        }
        if (airportCount < 2 || airportCount > MAX_ANALYSIS_AIRPORTS) {
            printf("Please choose a value between 2 and %d.\n", MAX_ANALYSIS_AIRPORTS);
            return;
        }

        for (int i = 0; i < airportCount; ++i) {
            printf("Airport %d code: ", i + 1);
            scanf("%31s", airportNames[i]);
        }
        for (int i = airportCount; i < MAX_ANALYSIS_AIRPORTS; ++i) {
            airportNames[i][0] = '\0';
        }

        int directedInput = 0;
        printf("Are routes directed? (1 = Yes, 0 = No): ");
        scanf("%d", &directedInput);

        int routeCount;
        printf("Enter number of flight routes: ");
        scanf("%d", &routeCount);
        if (routeCount <= 0) {
            printf("At least one route is required for analysis.\n");
            return;
        }

        graph = createRouteGraph(airportCount, directedInput ? 1 : 0);
        acceptedRoutes = 0;
        for (int index = 0; index < routeCount;) {
            char srcName[NAME_LEN];
            char destName[NAME_LEN];
            double weight;
            printf("\nRoute %d of %d\n", index + 1, routeCount);
            printf("  Source airport code: ");
            scanf("%31s", srcName);
            printf("  Destination airport code: ");
            scanf("%31s", destName);
            printf("  Distance/Cost (positive number): ");
            scanf("%lf", &weight);

            int srcIndex = findAirportIndex(airportNames, airportCount, srcName);
            int destIndex = findAirportIndex(airportNames, airportCount, destName);

            if (srcIndex == -1 || destIndex == -1) {
                printf("  Invalid airport code supplied. Please re-enter this route.\n");
                continue;
            }
            if (srcIndex == destIndex) {
                printf("  Source and destination must be different.\n");
                continue;
            }
            if (weight <= 0.0) {
                printf("  Weight must be a positive value.\n");
                continue;
            }

            if (graph.adjMatrix[srcIndex][destIndex] > weight) {
                graph.adjMatrix[srcIndex][destIndex] = weight;
            }
            if (!graph.directed && graph.adjMatrix[destIndex][srcIndex] > weight) {
                graph.adjMatrix[destIndex][srcIndex] = weight;
            }

            ++acceptedRoutes;
            ++index;
        }
    } else {
        printf("Unknown selection. Returning to main menu.\n");
        return;
    }

    RouteEdge* undirectedEdges = NULL;
    int undirectedEdgeCount = buildUndirectedEdgeList(&graph, &undirectedEdges);

    double maxEdges = graph.directed ? (double)airportCount * (airportCount - 1)
                                     : ((double)airportCount * (airportCount - 1)) / 2.0;
    double density = 0.0;
    if (airportCount > 1 && maxEdges > 0.0) {
        if (graph.directed) {
            density = ((double)acceptedRoutes) / maxEdges;
        } else {
            density = maxEdges > 0.0 ? ((double)undirectedEdgeCount) / maxEdges : 0.0;
        }
    }

    bool primSuccess = false;
    bool kruskalSuccess = false;
    double primWeight = 0.0;
    double kruskalWeight = 0.0;
    double primMs = 0.0;
    double kruskalMs = 0.0;
    int primEdgeCount = 0;
    int kruskalEdgeCount = 0;
    MSTResultEdge* primEdges = NULL;
    MSTResultEdge* kruskalEdges = NULL;

    if (airportCount >= 2 && undirectedEdgeCount >= airportCount - 1) {
        primEdges = (MSTResultEdge*)malloc(sizeof(MSTResultEdge) * (airportCount - 1));
        kruskalEdges = (MSTResultEdge*)malloc(sizeof(MSTResultEdge) * (airportCount - 1));

        clock_t startPrim = clock();
        primWeight = primMST(&graph, primEdges, &primEdgeCount);
        primMs = ((double)(clock() - startPrim) * 1000.0) / CLOCKS_PER_SEC;
        if (primEdgeCount == airportCount - 1 && primWeight < INF_WEIGHT / 2.0) {
            primSuccess = true;
        }

        clock_t startKruskal = clock();
        kruskalWeight = kruskalMST(&graph, undirectedEdges, undirectedEdgeCount, kruskalEdges, &kruskalEdgeCount);
        kruskalMs = ((double)(clock() - startKruskal) * 1000.0) / CLOCKS_PER_SEC;
        if (kruskalEdgeCount == airportCount - 1 && kruskalWeight < INF_WEIGHT / 2.0) {
            kruskalSuccess = true;
        }
    } else {
        printf("\nWarning: Not enough connectivity to build an MST. Add more routes.\n");
    }

    double** floydDist = allocateMatrix(airportCount, INF_WEIGHT);
    double** johnsonDist = allocateMatrix(airportCount, INF_WEIGHT);
    double floydMs = 0.0;
    double johnsonMs = 0.0;
    int floydStatus = 0;
    int johnsonStatus = 0;

    if (floydDist && johnsonDist) {
        clock_t startFloyd = clock();
        floydStatus = floydWarshallAllPairs(&graph, floydDist);
        floydMs = ((double)(clock() - startFloyd) * 1000.0) / CLOCKS_PER_SEC;

        clock_t startJohnson = clock();
        johnsonStatus = johnsonAllPairs(&graph, johnsonDist);
        johnsonMs = ((double)(clock() - startJohnson) * 1000.0) / CLOCKS_PER_SEC;
    }

    printGraphDiagram(&graph, airportNames);

    printf("\n--- Minimum Spanning Tree Analysis ---\n");
    printf("Graph density: %.3f -> %s graph\n", density, density >= 0.5 ? "dense" : "sparse");
    printf("%-30s %-12s %-15s %-15s\n", "Algorithm", "Time (ms)", "Total Weight", "Complexity");

    bool recommendPrim = density >= 0.5;
    bool mstUsePrim = recommendPrim;
    if (mstUsePrim && !primSuccess && kruskalSuccess) {
        mstUsePrim = false;
    } else if (!mstUsePrim && !kruskalSuccess && primSuccess) {
        mstUsePrim = true;
    }
    bool mstRecommendationPrinted = false;

    if (primSuccess) {
        printf("%-30s %12.3f %15.2f %-15s", "Prim's Algorithm", primMs, primWeight, "O(V^2)");
        if (recommendPrim) {
            printf("  <recommended>");
            mstRecommendationPrinted = true;
        }
        printf("\n");
    } else {
        printf("Prim's Algorithm could not produce an MST (graph disconnected).\n");
    }

    if (kruskalSuccess) {
        printf("%-30s %12.3f %15.2f %-15s", "Kruskal's Algorithm", kruskalMs, kruskalWeight, "O(E log V)");
        if (!mstRecommendationPrinted && !recommendPrim) {
            printf("  <recommended>");
        }
        printf("\n");
    } else {
        printf("Kruskal's Algorithm could not produce an MST (graph disconnected).\n");
    }

    if (mstUsePrim && primSuccess) {
        printf("\nSelected MST: Prim's Algorithm\n");
        for (int i = 0; i < primEdgeCount; ++i) {
            printf("  %s -- %s : %.2f\n", airportNames[primEdges[i].src], airportNames[primEdges[i].dest], primEdges[i].weight);
        }
    } else if (!mstUsePrim && kruskalSuccess) {
        printf("\nSelected MST: Kruskal's Algorithm\n");
        for (int i = 0; i < kruskalEdgeCount; ++i) {
            printf("  %s -- %s : %.2f\n", airportNames[kruskalEdges[i].src], airportNames[kruskalEdges[i].dest], kruskalEdges[i].weight);
        }
    }
    if (recommendPrim && !mstUsePrim && kruskalSuccess) {
        printf("Note: Dense-graph recommendation fell back to Kruskal because Prim could not build a complete MST.\n");
    }
    if (!recommendPrim && mstUsePrim && primSuccess && !kruskalSuccess) {
        printf("Note: Sparse-graph recommendation fell back to Prim because Kruskal could not build a complete MST.\n");
    }

    printf("\n--- All-Pairs Shortest Paths ---\n");
    bool recommendFloyd = (airportCount <= APSP_FLOYD_THRESHOLD);
    printf("Airport count: %d -> %s recommended\n", airportCount, recommendFloyd ? "Floyd-Warshall" : "Johnson + Dijkstra");
    printf("%-30s %-12s %-15s\n", "Algorithm", "Time (ms)", "Complexity");

    bool apspUseFloyd = recommendFloyd;
    if (apspUseFloyd && floydStatus != 0 && johnsonStatus == 0) {
        apspUseFloyd = false;
    } else if (!apspUseFloyd && johnsonStatus != 0 && floydStatus == 0) {
        apspUseFloyd = true;
    }

    if (floydStatus == 0) {
        printf("%-30s %12.3f %-15s", "Floyd-Warshall", floydMs, "O(V^3)");
        if (recommendFloyd) {
            printf("  <recommended>");
        }
        printf("\n");
    } else {
        printf("Floyd-Warshall encountered an error.\n");
    }

    if (johnsonStatus == 0) {
        printf("%-30s %12.3f %-15s", "Johnson + Dijkstra", johnsonMs, "O(VE + V log V)");
        if (!recommendFloyd) {
            printf("  <recommended>");
        }
        printf("\n");
    } else {
        printf("Johnson's algorithm detected a negative cycle or failed.\n");
    }

    if (apspUseFloyd && floydStatus == 0) {
        printf("\nShortest path matrix (Floyd-Warshall):\n");
        printDistanceMatrix(floydDist, airportCount, airportNames);
    } else if (!apspUseFloyd && johnsonStatus == 0) {
        printf("\nShortest path matrix (Johnson + Dijkstra):\n");
        printDistanceMatrix(johnsonDist, airportCount, airportNames);
    }
    if (recommendFloyd && !apspUseFloyd && johnsonStatus == 0) {
        printf("Note: Floyd-Warshall was recommended, but Johnson's algorithm delivered the valid paths.\n");
    }
    if (!recommendFloyd && apspUseFloyd && floydStatus == 0) {
        printf("Note: Johnson's path computation was unavailable, so Floyd-Warshall was used instead.\n");
    }

    freeRouteGraph(&graph);
    if (undirectedEdges) free(undirectedEdges);
    if (primEdges) free(primEdges);
    if (kruskalEdges) free(kruskalEdges);
    freeMatrix(floydDist, airportCount);
    freeMatrix(johnsonDist, airportCount);
    printf("\nRoute analysis complete.\n");
}

static RouteGraph createRouteGraph(int vertices, int directed) {
    RouteGraph graph;
    graph.vertexCount = vertices;
    graph.directed = directed ? 1 : 0;
    graph.adjMatrix = allocateMatrix(vertices, INF_WEIGHT);
    if (graph.adjMatrix) {
        for (int i = 0; i < vertices; ++i) {
            graph.adjMatrix[i][i] = 0.0;
        }
    }
    return graph;
}

static void freeRouteGraph(RouteGraph* graph) {
    if (!graph || !graph->adjMatrix) return;
    freeMatrix(graph->adjMatrix, graph->vertexCount);
    graph->adjMatrix = NULL;
}

static double** allocateMatrix(int n, double initial) {
    if (n <= 0) return NULL;
    double** matrix = (double**)malloc(sizeof(double*) * n);
    if (!matrix) return NULL;
    for (int i = 0; i < n; ++i) {
        matrix[i] = (double*)malloc(sizeof(double) * n);
        if (!matrix[i]) {
            for (int k = 0; k < i; ++k) {
                free(matrix[k]);
            }
            free(matrix);
            return NULL;
        }
        for (int j = 0; j < n; ++j) {
            matrix[i][j] = initial;
        }
    }
    return matrix;
}

static void freeMatrix(double** matrix, int n) {
    if (!matrix) return;
    for (int i = 0; i < n; ++i) {
        free(matrix[i]);
    }
    free(matrix);
}

static void printGraphDiagram(const RouteGraph* graph, char airportNames[][NAME_LEN]) {
    if (!graph || !graph->adjMatrix) {
        printf("Graph not available.\n");
        return;
    }
    printf("\nRoute weight matrix (%s):\n", graph->directed ? "directed" : "undirected");
    printf("%-12s", " ");
    for (int j = 0; j < graph->vertexCount; ++j) {
        printf("%-12s", airportNames[j]);
    }
    printf("\n");
    for (int i = 0; i < graph->vertexCount; ++i) {
        printf("%-12s", airportNames[i]);
        for (int j = 0; j < graph->vertexCount; ++j) {
            if (graph->adjMatrix[i][j] >= INF_WEIGHT / 2.0) {
                printf("%-12s", "INF");
            } else {
                printf("%-12.2f", graph->adjMatrix[i][j]);
            }
        }
        printf("\n");
    }
}

static int findAirportIndex(char airportNames[][NAME_LEN], int count, const char* name) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(airportNames[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

static double primMST(const RouteGraph* graph, MSTResultEdge* output, int* edgeCount) {
    if (!graph || !graph->adjMatrix || graph->vertexCount <= 1) {
        *edgeCount = 0;
        return INF_WEIGHT;
    }
    int V = graph->vertexCount;
    double* key = (double*)malloc(sizeof(double) * V);
    int* parent = (int*)malloc(sizeof(int) * V);
    bool* inMST = (bool*)malloc(sizeof(bool) * V);
    if (!key || !parent || !inMST) {
        free(key); free(parent); free(inMST);
        *edgeCount = 0;
        return INF_WEIGHT;
    }
    for (int i = 0; i < V; ++i) {
        key[i] = INF_WEIGHT;
        parent[i] = -1;
        inMST[i] = false;
    }
    key[0] = 0.0;

    for (int count = 0; count < V - 1; ++count) {
        double min = INF_WEIGHT;
        int u = -1;
        for (int v = 0; v < V; ++v) {
            if (!inMST[v] && key[v] < min) {
                min = key[v];
                u = v;
            }
        }
        if (u == -1) {
            free(key); free(parent); free(inMST);
            *edgeCount = 0;
            return INF_WEIGHT;
        }
        inMST[u] = true;
        for (int v = 0; v < V; ++v) {
            double weight = graph->adjMatrix[u][v];
            if (!inMST[v] && weight < key[v]) {
                key[v] = weight;
                parent[v] = u;
            }
        }
    }

    double total = 0.0;
    int idx = 0;
    for (int v = 1; v < V; ++v) {
        if (parent[v] == -1 || graph->adjMatrix[parent[v]][v] >= INF_WEIGHT / 2.0) {
            free(key); free(parent); free(inMST);
            *edgeCount = 0;
            return INF_WEIGHT;
        }
        output[idx].src = parent[v];
        output[idx].dest = v;
        output[idx].weight = graph->adjMatrix[parent[v]][v];
        total += output[idx].weight;
        ++idx;
    }

    *edgeCount = idx;
    free(key); free(parent); free(inMST);
    return total;
}

static double kruskalMST(const RouteGraph* graph, RouteEdge* edges, int edgeCount, MSTResultEdge* output, int* mstEdgeCount) {    
    int V = graph->vertexCount;
    int parent[100];
    int count = 0;
    double weight = 0;
    
    // Sort edges
    qsort(edges, edgeCount, sizeof(RouteEdge), compareRouteEdges);
    
    // Initialize
    for (int i = 0; i < V; i++) {
        parent[i] = i;
    }
    
    // Process edges
    for (int i = 0; i < edgeCount; i++) {
        
        int u = edges[i].src;
        int v = edges[i].dest;
        
        // Find root of u
        int ru = u;
        while (parent[ru] != ru) ru = parent[ru];
        
        // Find root of v
        int rv = v;
        while (parent[rv] != rv) rv = parent[rv];
        
        // Different roots? Add edge
        if (ru != rv) {
            output[count].src = edges[i].src;
            output[count].dest = edges[i].dest;
            output[count].weight = edges[i].weight;
            weight = weight + edges[i].weight;
            count = count + 1;
            parent[ru] = rv;
        }
    }
    
    *mstEdgeCount = count;
    return weight;
}

static int floydWarshallAllPairs(const RouteGraph* graph, double** distOut) {
    if (!graph || !graph->adjMatrix || !distOut) return -1;
    int V = graph->vertexCount;
    for (int i = 0; i < V; ++i) {
        for (int j = 0; j < V; ++j) {
            distOut[i][j] = graph->adjMatrix[i][j];
        }
        distOut[i][i] = 0.0;
    }

    for (int k = 0; k < V; ++k) {
        for (int i = 0; i < V; ++i) {
            if (distOut[i][k] >= INF_WEIGHT / 2.0) continue;
            for (int j = 0; j < V; ++j) {
                if (distOut[k][j] >= INF_WEIGHT / 2.0) continue;
                double alt = distOut[i][k] + distOut[k][j];
                if (alt < distOut[i][j]) {
                    distOut[i][j] = alt;
                }
            }
        }
    }
    return 0;
}

static int johnsonAllPairs(const RouteGraph* graph, double** distOut) {
    if (!graph || !graph->adjMatrix || !distOut) return -1;
    int V = graph->vertexCount;
    int maxPossibleEdges = V * V;
    RouteEdge* edges = (RouteEdge*)malloc(sizeof(RouteEdge) * maxPossibleEdges);
    if (!edges) return -1;
    int edgeCount = 0;
    for (int i = 0; i < V; ++i) {
        for (int j = 0; j < V; ++j) {
            if (i == j) continue;
            if (graph->adjMatrix[i][j] < INF_WEIGHT / 2.0) {
                edges[edgeCount].src = i;
                edges[edgeCount].dest = j;
                edges[edgeCount].weight = graph->adjMatrix[i][j];
                ++edgeCount;
            }
        }
    }

    int totalEdges = edgeCount + V;
    RouteEdge* bfEdges = (RouteEdge*)malloc(sizeof(RouteEdge) * totalEdges);
    if (!bfEdges) {
        free(edges);
        return -1;
    }
    for (int i = 0; i < edgeCount; ++i) {
        bfEdges[i] = edges[i];
    }
    for (int v = 0; v < V; ++v) {
        bfEdges[edgeCount + v].src = V;
        bfEdges[edgeCount + v].dest = v;
        bfEdges[edgeCount + v].weight = 0.0;
    }

    double* h = (double*)malloc(sizeof(double) * (V + 1));
    if (!h) {
        free(edges);
        free(bfEdges);
        return -1;
    }
    for (int i = 0; i <= V; ++i) h[i] = 0.0;

    for (int i = 0; i < V; ++i) {
        bool updated = false;
        for (int e = 0; e < totalEdges; ++e) {
            int u = bfEdges[e].src;
            int v = bfEdges[e].dest;
            double w = bfEdges[e].weight;
            if (h[u] + w < h[v]) {
                h[v] = h[u] + w;
                updated = true;
            }
        }
        if (!updated) break;
        if (i == V - 1 && updated) {
            free(edges);
            free(bfEdges);
            free(h);
            return -1;
        }
    }

    for (int e = 0; e < totalEdges; ++e) {
        int u = bfEdges[e].src;
        int v = bfEdges[e].dest;
        double w = bfEdges[e].weight;
        if (h[u] + w < h[v]) {
            free(edges);
            free(bfEdges);
            free(h);
            return -1;
        }
    }

    double** reweighted = allocateMatrix(V, INF_WEIGHT);
    if (!reweighted) {
        free(edges);
        free(bfEdges);
        free(h);
        return -1;
    }
    for (int i = 0; i < V; ++i) {
        for (int j = 0; j < V; ++j) {
            if (graph->adjMatrix[i][j] < INF_WEIGHT / 2.0) {
                reweighted[i][j] = graph->adjMatrix[i][j] + h[i] - h[j];
            }
        }
    }

    double* tempDist = (double*)malloc(sizeof(double) * V);
    if (!tempDist) {
        free(edges);
        free(bfEdges);
        free(h);
        freeMatrix(reweighted, V);
        return -1;
    }

    for (int i = 0; i < V; ++i) {
        dijkstraReweighted(graph, reweighted, i, tempDist);
        for (int j = 0; j < V; ++j) {
            if (tempDist[j] >= INF_WEIGHT / 2.0) {
                distOut[i][j] = INF_WEIGHT;
            } else {
                distOut[i][j] = tempDist[j] + h[j] - h[i];
            }
        }
    }

    free(edges);
    free(bfEdges);
    free(h);
    freeMatrix(reweighted, V);
    free(tempDist);
    return 0;
}

static void printDistanceMatrix(double** dist, int n, char airportNames[][NAME_LEN]) {
    printf("%-12s", " ");
    for (int j = 0; j < n; ++j) {
        printf("%-12s", airportNames[j]);
    }
    printf("\n");
    for (int i = 0; i < n; ++i) {
        printf("%-12s", airportNames[i]);
        for (int j = 0; j < n; ++j) {
            if (dist[i][j] >= INF_WEIGHT / 2.0) {
                printf("%-12s", "INF");
            } else {
                printf("%-12.2f", dist[i][j]);
            }
        }
        printf("\n");
    }
}

static int buildUndirectedEdgeList(const RouteGraph* graph, RouteEdge** edgesOut) {
    if (!graph || !graph->adjMatrix) {
        *edgesOut = NULL;
        return 0;
    }
    int V = graph->vertexCount;
    int capacity = V * (V - 1) / 2;
    if (capacity <= 0) {
        *edgesOut = NULL;
        return 0;
    }
    RouteEdge* edges = (RouteEdge*)malloc(sizeof(RouteEdge) * capacity);
    if (!edges) {
        *edgesOut = NULL;
        return 0;
    }
    int count = 0;
    for (int i = 0; i < V; ++i) {
        for (int j = i + 1; j < V; ++j) {
            double wForward = graph->adjMatrix[i][j];
            double wBackward = graph->adjMatrix[j][i];
            double weight = wForward;
            if (wBackward < weight) weight = wBackward;
            if (weight < INF_WEIGHT / 2.0) {
                edges[count].src = i;
                edges[count].dest = j;
                edges[count].weight = weight;
                ++count;
            }
        }
    }
    *edgesOut = edges;
    return count;
}

static void dijkstraReweighted(const RouteGraph* graph, double** reweighted, int src, double* dist) {
    int V = graph->vertexCount;
    bool* visited = (bool*)calloc(V, sizeof(bool));
    for (int i = 0; i < V; ++i) {
        dist[i] = INF_WEIGHT;
    }
    dist[src] = 0.0;

    for (int count = 0; count < V; ++count) {
        double minDist = INF_WEIGHT;
        int u = -1;
        for (int v = 0; v < V; ++v) {
            if (!visited[v] && dist[v] < minDist) {
                minDist = dist[v];
                u = v;
            }
        }
        if (u == -1) break;
        visited[u] = true;
        for (int v = 0; v < V; ++v) {
            if (reweighted[u][v] >= INF_WEIGHT / 2.0) continue;
            double alt = dist[u] + reweighted[u][v];
            if (alt < dist[v]) {
                dist[v] = alt;
            }
        }
    }

    free(visited);
}

static int disjointSetFind(int* parent, int v) {
    if (parent[v] != v) {
        parent[v] = disjointSetFind(parent, parent[v]);
    }
    return parent[v];
}

static void disjointSetUnion(int* parent, int* rank, int a, int b) {
    a = disjointSetFind(parent, a);
    b = disjointSetFind(parent, b);
    if (a == b) return;
    if (rank[a] < rank[b]) {
        parent[a] = b;
    } else if (rank[a] > rank[b]) {
        parent[b] = a;
    } else {
        parent[b] = a;
        rank[a]++;
    }
}

static int compareRouteEdges(const void* a, const void* b) {
    const RouteEdge* ea = (const RouteEdge*)a;
    const RouteEdge* eb = (const RouteEdge*)b;
    if (ea->weight < eb->weight) return -1;
    if (ea->weight > eb->weight) return 1;
    return 0;
}
