#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#pragma comment(lib, "ws2_32.lib")

#define MAX_PATIENTS 10
#define BUFFER_SIZE 1024

int appointments[20] = {0};
int patients[MAX_PATIENTS] = {-1};
int patient_count = 0;
CRITICAL_SECTION patient_lock;
static int patient_counter = 0;

void send_message(int socket, const char* message) {
    send(socket, message, strlen(message), 0);
}

void view_appointments(int patient_socket) {
    char message[BUFFER_SIZE] = "Available Appointments:\n";
    char appointment_info[32];
    for (int i = 0; i < 4; i++) {
        snprintf(appointment_info, sizeof(appointment_info), "Doctor %d:\n", i + 1);
        strcat(message, appointment_info);
        for (int j = 0; j < 5; j++) {
            int index = i * 5 + j;
            if (appointments[index] == 0) {
                snprintf(appointment_info, sizeof(appointment_info), "  [ %d ]\n", j + 1);
                strcat(message, appointment_info);
            }
        }
    }
    send_message(patient_socket, message);
}

void book_appointment(int patient_socket, int patient_id) {
    char message[64];
    int doctor_id, appointment_id;
    
    send_message(patient_socket, "Enter doctor number (1-4): ");
    recv(patient_socket, message, sizeof(message), 0);
    doctor_id = atoi(message) - 1;

    if (doctor_id < 0 || doctor_id >= 4) {
        send_message(patient_socket, "Invalid doctor number.\n");
        return;
    }

    send_message(patient_socket, "Enter appointment number (1-5): ");
    recv(patient_socket, message, sizeof(message), 0);
    appointment_id = atoi(message) - 1;

    if (appointment_id < 0 || appointment_id >= 5) {
        send_message(patient_socket, "Invalid appointment number.\n");
        return;
    }

    int index = doctor_id * 5 + appointment_id;
    if (appointments[index] == 0) {
        appointments[index] = patient_id;
        snprintf(message, sizeof(message), "Appointment %d with Doctor %d booked for you.\n", 
                appointment_id + 1, doctor_id + 1);
        send_message(patient_socket, message);
    } else {
        send_message(patient_socket, "This appointment is already booked.\n");
    }
}

void cancel_appointment(int patient_socket, int patient_id) {
    int had_appointment = 0;
    for (int i = 0; i < 20; i++) {
        if (appointments[i] == patient_id) {
            appointments[i] = 0;
            had_appointment = 1;
        }
    }
    
    char response[64];
    if (had_appointment) {
        strcpy(response, "Your appointment has been canceled.\n");
    } else {
        strcpy(response, "You don't have any appointments to cancel.\n");
    }
    send_message(patient_socket, response);
}

unsigned __stdcall handle_patient(void* patient_socket_ptr) {
    int patient_socket = *(int*)patient_socket_ptr;
    free(patient_socket_ptr);

    char buffer[BUFFER_SIZE];
    int read_size;
    int patient_id = patient_socket;

    patient_counter++;
    printf("New patient connected: Patient %d\n", patient_counter);

    send_message(patient_socket, "Welcome to the Doctor Appointment System!\n");
    send_message(patient_socket, "Please choose an option:\n");
    send_message(patient_socket, "1. View Appointments\n");
    send_message(patient_socket, "2. Book Appointment\n");
    send_message(patient_socket, "3. Cancel Appointment\n");
    send_message(patient_socket, "4. Exit\n");

    while ((read_size = recv(patient_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[read_size] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';
        printf("Patient %d command: %s\n", patient_id, buffer);

        if (strcmp(buffer, "4") == 0) {
            printf("Patient %d disconnected.\n", patient_id);
            cancel_appointment(patient_socket, patient_id);
            break;
        } else if (strcmp(buffer, "1") == 0) {
            view_appointments(patient_socket);
        } else if (strcmp(buffer, "2") == 0) {
            book_appointment(patient_socket, patient_id);
        } else if (strcmp(buffer, "3") == 0) {
            cancel_appointment(patient_socket, patient_id);
        } else {
            send_message(patient_socket, "Invalid command. Please enter a number between 1 and 4.\n");
        }
    }

    EnterCriticalSection(&patient_lock);
    for (int i = 0; i < MAX_PATIENTS; i++) {
        if (patients[i] == patient_socket) {
            patients[i] = -1;
            patient_count--;
            break;
        }
    }
    LeaveCriticalSection(&patient_lock);

    closesocket(patient_socket);
    return 0;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    InitializeCriticalSection(&patient_lock);

    int server_socket, patient_socket;
    struct sockaddr_in server_addr, patient_addr;
    int addr_size = sizeof(struct sockaddr_in);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 3) == SOCKET_ERROR) {
        printf("Listen failed with error: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Waiting for incoming connections...\n");

    while ((patient_socket = accept(server_socket, (struct sockaddr*)&patient_addr, &addr_size)) != INVALID_SOCKET) {
        printf("Connection accepted\n");

        int* new_sock = malloc(sizeof(int));
        *new_sock = patient_socket;

        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, handle_patient, new_sock, 0, NULL);
        if (thread == 0) {
            printf("Could not create thread\n");
            free(new_sock);
            continue;
        }
        CloseHandle(thread);
        printf("Handler assigned\n");
    }

    if (patient_socket == INVALID_SOCKET) {
        printf("Accept failed with error: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    DeleteCriticalSection(&patient_lock);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}