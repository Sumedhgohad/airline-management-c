
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

/* Keep a single built-in demo scenario for simplicity — 6 Indian airports (undirected). */
static const char* const SAMPLE_SMALL_APSP_NAMES[] = {"DEL", "BOM", "BLR", "HYD", "CCU", "PNQ"};
static const SampleEdge SAMPLE_SMALL_APSP_EDGES[] = {
    {"DEL", "BOM", 720}, {"DEL", "BLR", 1740}, {"DEL", "HYD", 1265}, {"DEL", "CCU", 1300}, {"DEL", "PNQ", 1440},
    {"BOM", "BLR", 984}, {"BOM", "HYD", 710}, {"BOM", "CCU", 1650}, {"BOM", "PNQ", 148},
    {"BLR", "HYD", 550}, {"BLR", "CCU", 1515}, {"BLR", "PNQ", 840},
    {"HYD", "CCU", 1180}, {"HYD", "PNQ", 620},
    {"CCU", "PNQ", 1655}
};

static RouteGraph createRouteGraph(int vertices, int directed);
static void freeRouteGraph(RouteGraph* graph);
static double** allocateMatrix(int n, double initial);
static void freeMatrix(double** matrix, int n);
static void printGraphDiagram(const RouteGraph* graph, char airportNames[][NAME_LEN]);
static int findAirportIndex(char airportNames[][NAME_LEN], int count, const char* name);
static double primMST(const RouteGraph* graph, MSTResultEdge* output, int* edgeCount);
static double kruskalMST(const RouteGraph* graph, RouteEdge* edges, int edgeCount, MSTResultEdge* output, int* mstEdgeCount);
static int buildUndirectedEdgeList(const RouteGraph* graph, RouteEdge** edgesOut);
static int disjointSetFind(int* parent, int v);
static void disjointSetUnion(int* parent, int* rank, int a, int b);
static int compareRouteEdges(const void* a, const void* b);
static int floydWarshallAllPairs(const RouteGraph* graph, double** distOut);
static void dijkstra(const RouteGraph* graph, int src, double* dist);
static void printDistanceMatrix(double** dist, int n, char airportNames[][NAME_LEN]);
/* We keep a single small built-in demo; no multiple-scenario helpers required. */

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
    newNode->next=
    
    NULL;
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
    printf("| 8. Analyze Route Network            |\n");
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
            case 8: analyzeRouteNetwork(); break;
            case 0:
                printf("Thank you for using Airline Management System!\n");
                exit(0);
            default:
                printf("Invalid choice! Please try again.\n");
        }
    }
}

/* Multiple-scenario helpers removed — we use a single built-in demo dataset now. */

void analyzeRouteNetwork() {
    printf("\n=== ROUTE NETWORK ANALYSIS ===\n");
    printf("Using built-in demo dataset: Regional shuttle circuit (small)\n");

    RouteGraph graph;
    graph.vertexCount = 0;
    graph.directed = 0; /* demo uses undirected Indian network */
    graph.adjMatrix = NULL;

    int airportCount = (int)(sizeof(SAMPLE_SMALL_APSP_NAMES) / sizeof(SAMPLE_SMALL_APSP_NAMES[0]));
    int acceptedRoutes = 0;
    char airportNames[MAX_ANALYSIS_AIRPORTS][NAME_LEN];

    graph = createRouteGraph(airportCount, 0);
    if (!graph.adjMatrix) {
        printf("Unable to allocate memory for demo dataset.\n");
        return;
    }
    for (int i = 0; i < airportCount; ++i) {
        strncpy(airportNames[i], SAMPLE_SMALL_APSP_NAMES[i], NAME_LEN - 1);
        airportNames[i][NAME_LEN - 1] = '\0';
    }
    for (int i = airportCount; i < MAX_ANALYSIS_AIRPORTS; ++i) airportNames[i][0] = '\0';

    int edgeCount = (int)(sizeof(SAMPLE_SMALL_APSP_EDGES) / sizeof(SAMPLE_SMALL_APSP_EDGES[0]));
    for (int i = 0; i < edgeCount; ++i) {
        int s = findAirportIndex(airportNames, airportCount, SAMPLE_SMALL_APSP_EDGES[i].src);
        int d = findAirportIndex(airportNames, airportCount, SAMPLE_SMALL_APSP_EDGES[i].dest);
        if (s == -1 || d == -1) continue;
        double w = SAMPLE_SMALL_APSP_EDGES[i].weight;
        if (graph.adjMatrix[s][d] > w) graph.adjMatrix[s][d] = w;
        if (!graph.directed && graph.adjMatrix[d][s] > w) graph.adjMatrix[d][s] = w;
        ++acceptedRoutes;
    }
    printf("Airports: %d  Routes: %d  Directed: %s\n", airportCount, acceptedRoutes, graph.directed ? "Yes" : "No");

    /* Build undirected edge list (needed if user chooses Kruskal) and APSP buffer */
    RouteEdge* undirectedEdges = NULL;
    int undirectedEdgeCount = buildUndirectedEdgeList(&graph, &undirectedEdges);
    double** floydDist = allocateMatrix(airportCount, INF_WEIGHT);

    printGraphDiagram(&graph, airportNames);

    /* --- Minimum Spanning Tree: let user choose which algorithm to run --- */
    printf("\n--- Minimum Spanning Tree (MST) ---\n");
    printf("Choose MST algorithm to run:\n");
    printf("  1. Prim's Algorithm\n");
    printf("  2. Kruskal's Algorithm\n");
    printf("  3. Skip MST\n");
    printf("Enter choice: ");
    int mstChoice = 0;
    if (scanf("%d", &mstChoice) != 1) mstChoice = 3;

    if (mstChoice == 1) {
        if (airportCount < 2) {
            printf("Not enough vertices to run Prim's algorithm.\n");
        } else {
            MSTResultEdge* primEdges = (MSTResultEdge*)malloc(sizeof(MSTResultEdge) * (airportCount - 1));
            int primEdgeCount = 0;
            clock_t startPrim = clock();
            double primWeight = primMST(&graph, primEdges, &primEdgeCount);
            double primMs = ((double)(clock() - startPrim) * 1000.0) / CLOCKS_PER_SEC;
            if (primEdgeCount == airportCount - 1 && primWeight < INF_WEIGHT / 2.0) {
                printf("Prim's Algorithm succeeded (Time: %.3f ms, Total weight: %.2f)\n", primMs, primWeight);
                for (int i = 0; i < primEdgeCount; ++i) {
                    printf("  %s -- %s : %.2f\n", airportNames[primEdges[i].src], airportNames[primEdges[i].dest], primEdges[i].weight);
                }
            } else {
                printf("Prim's Algorithm could not produce an MST (graph disconnected).\n");
            }
            free(primEdges);
        }
    } else if (mstChoice == 2) {
        if (undirectedEdgeCount < airportCount - 1) {
            printf("Not enough undirected edges to run Kruskal's algorithm.\n");
        } else {
            MSTResultEdge* kruskalEdges = (MSTResultEdge*)malloc(sizeof(MSTResultEdge) * (airportCount - 1));
            int kruskalEdgeCount = 0;
            clock_t startKruskal = clock();
            double kruskalWeight = kruskalMST(&graph, undirectedEdges, undirectedEdgeCount, kruskalEdges, &kruskalEdgeCount);
            double kruskalMs = ((double)(clock() - startKruskal) * 1000.0) / CLOCKS_PER_SEC;
            if (kruskalEdgeCount == airportCount - 1 && kruskalWeight < INF_WEIGHT / 2.0) {
                printf("Kruskal's Algorithm succeeded (Time: %.3f ms, Total weight: %.2f)\n", kruskalMs, kruskalWeight);
                for (int i = 0; i < kruskalEdgeCount; ++i) {
                    printf("  %s -- %s : %.2f\n", airportNames[kruskalEdges[i].src], airportNames[kruskalEdges[i].dest], kruskalEdges[i].weight);
                }
            } else {
                printf("Kruskal's Algorithm could not produce an MST (graph disconnected).\n");
            }
            free(kruskalEdges);
        }
    } else {
        printf("Skipping MST analysis as requested.\n");
    }

    /* --- Shortest paths: let user choose Floyd-Warshall (all-pairs) or single-source Dijkstra --- */
    printf("\n--- Shortest Paths ---\n");
    printf("Choose algorithm:\n");
    printf("  1. Floyd-Warshall (all-pairs)\n");
    printf("  2. Single-source Dijkstra\n");
    printf("  3. Skip shortest-paths\n");
    printf("Enter choice: ");
    int spChoice = 0;
    if (scanf("%d", &spChoice) != 1) spChoice = 3;

    if (spChoice == 1) {
        if (!floydDist) {
            printf("Unable to allocate memory for Floyd-Warshall.\n");
        } else {
            clock_t startFloyd = clock();
            int floydStatus = floydWarshallAllPairs(&graph, floydDist);
            double floydMs = ((double)(clock() - startFloyd) * 1000.0) / CLOCKS_PER_SEC;
            if (floydStatus == 0) {
                printf("Floyd-Warshall completed (Time: %.3f ms)\n", floydMs);
                printf("\nShortest path matrix (Floyd-Warshall):\n");
                printDistanceMatrix(floydDist, airportCount, airportNames);
            } else {
                printf("Floyd-Warshall encountered an error.\n");
            }
        }
    } else if (spChoice == 2) {
        printf("Enter source airport code: ");
        char srcName[NAME_LEN];
        scanf("%31s", srcName);
        int srcIndex = findAirportIndex(airportNames, airportCount, srcName);
        if (srcIndex == -1) {
            printf("Unknown airport code.\n");
        } else {
            double* dist = (double*)malloc(sizeof(double) * airportCount);
            if (!dist) {
                printf("Allocation failed for Dijkstra.\n");
            } else {
                clock_t startD = clock();
                dijkstra(&graph, srcIndex, dist);
                double dMs = ((double)(clock() - startD) * 1000.0) / CLOCKS_PER_SEC;
                printf("Dijkstra from %s completed (Time: %.3f ms)\n", airportNames[srcIndex], dMs);
                printf("%-12s %-12s\n", "Airport", "Distance");
                for (int i = 0; i < airportCount; ++i) {
                    if (dist[i] >= INF_WEIGHT / 2.0) {
                        printf("%-12s %-12s\n", airportNames[i], "INF");
                    } else {
                        printf("%-12s %-12.2f\n", airportNames[i], dist[i]);
                    }
                }
            }
            free(dist);
        }
    } else {
        printf("Skipping shortest-paths as requested.\n");
    }

    freeRouteGraph(&graph);
    if (undirectedEdges) free(undirectedEdges);
    freeMatrix(floydDist, airportCount);
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

static double kruskalMST(const RouteGraph* graph, RouteEdge* edges, int edgeCount, MSTResultEdge* output, int* mstEdgeCount) {
    if (!graph || !edges || edgeCount <= 0) {
        *mstEdgeCount = 0;
        return INF_WEIGHT;
    }
    int V = graph->vertexCount;
    qsort(edges, edgeCount, sizeof(RouteEdge), compareRouteEdges);
    int* parent = (int*)malloc(sizeof(int) * V);
    int* rank = (int*)calloc(V, sizeof(int));
    if (!parent || !rank) {
        free(parent); free(rank);
        *mstEdgeCount = 0;
        return INF_WEIGHT;
    }
    for (int i = 0; i < V; ++i) parent[i] = i;

    int added = 0;
    double total = 0.0;
    for (int i = 0; i < edgeCount && added < V - 1; ++i) {
        int u = edges[i].src;
        int v = edges[i].dest;
        int setU = disjointSetFind(parent, u);
        int setV = disjointSetFind(parent, v);
        if (setU != setV) {
            output[added].src = u;
            output[added].dest = v;
            output[added].weight = edges[i].weight;
            total += edges[i].weight;
            ++added;
            disjointSetUnion(parent, rank, setU, setV);
        }
    }

    free(parent);
    free(rank);

    if (added != V - 1) {
        *mstEdgeCount = 0;
        return INF_WEIGHT;
    }
    *mstEdgeCount = added;
    return total;
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

/* Simple Dijkstra (single-source) using adjacency matrix (non-negative weights only) */
static void dijkstra(const RouteGraph* graph, int src, double* dist) {
    int V = graph->vertexCount;
    bool* visited = (bool*)calloc(V, sizeof(bool));
    for (int i = 0; i < V; ++i) dist[i] = INF_WEIGHT;
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
            double w = graph->adjMatrix[u][v];
            if (w >= INF_WEIGHT / 2.0) continue;
            double alt = dist[u] + w;
            if (alt < dist[v]) dist[v] = alt;
        }
    }
    free(visited);
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

/* Johnson removed to simplify the program (keep Floyd-Warshall only) */

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

/* Undirected edge list builder removed (not needed without Kruskal) */

/* dijkstraReweighted removed (used only by Johnson) */

/* Disjoint-set helpers removed (used only by Kruskal) */

/* Edge comparator removed (used only by Kruskal) */
