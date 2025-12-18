#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_PATIENTS 10
#define BUFFER_SIZE 1024

int appointments[20] = {0};
CRITICAL_SECTION lock; // Randevular için kilit

void send_message(int socket, const char* message) {
    send(socket, message, (int)strlen(message), 0);
}

void view_appointments(int patient_socket) {
    char message[BUFFER_SIZE] = "Mevcut Randevular:\n";
    char info[64];
    EnterCriticalSection(&lock);
    for (int i = 0; i < 4; i++) {
        sprintf(info, "Doktor %d:\n", i + 1);
        strcat(message, info);
        for (int j = 0; j < 5; j++) {
            int idx = i * 5 + j;
            if (appointments[idx] == 0) {
                sprintf(info, "  [ %d ]\n", j + 1);
                strcat(message, info);
            }
        }
    }
    LeaveCriticalSection(&lock);
    send_message(patient_socket, message);
}

void book_appointment(int patient_socket, int patient_id) {
    char msg[BUFFER_SIZE];
    int doc, app;
    
    send_message(patient_socket, "Doktor no (1-4): ");
    recv(patient_socket, msg, sizeof(msg), 0);
    doc = atoi(msg) - 1;

    send_message(patient_socket, "Randevu no (1-5): ");
    recv(patient_socket, msg, sizeof(msg), 0);
    app = atoi(msg) - 1;

    if (doc < 0 || doc >= 4 || app < 0 || app >= 5) {
        send_message(patient_socket, "Gecersiz secim!");
        return;
    }

    EnterCriticalSection(&lock);
    int idx = doc * 5 + app;
    if (appointments[idx] == 0) {
        appointments[idx] = patient_id;
        sprintf(msg, "Doktor %d, Randevu %d basariyla alindi.", doc+1, app+1);
    } else {
        strcpy(msg, "Bu slot dolu!");
    }
    LeaveCriticalSection(&lock);
    send_message(patient_socket, msg);
}

void cancel_appointment(int patient_socket, int patient_id) {
    int count = 0;
    EnterCriticalSection(&lock);
    for (int i = 0; i < 20; i++) {
        if (appointments[i] == patient_id) {
            appointments[i] = 0;
            count++;
        }
    }
    LeaveCriticalSection(&lock);
    
    if (count > 0) send_message(patient_socket, "Randevunuz iptal edildi.");
    else send_message(patient_socket, "Iptal edilecek randevunuz yok.");
}

unsigned __stdcall handle_patient(void* socket_ptr) {
    int sock = *(int*)socket_ptr;
    free(socket_ptr);
    char buf[BUFFER_SIZE];
    int id = sock; // Soket ID'sini hasta kimligi olarak kullaniyoruz

    while (recv(sock, buf, sizeof(buf), 0) > 0) {
        int cmd = atoi(buf);
        if (cmd == 1) view_appointments(sock);
        else if (cmd == 2) book_appointment(sock, id);
        else if (cmd == 3) cancel_appointment(sock, id);
        else if (cmd == 4) break;
    }
    
    // Baglanti kopunca sadece bu hastanin randevularini temizle
    EnterCriticalSection(&lock);
    for (int i = 0; i < 20; i++) if (appointments[i] == id) appointments[i] = 0;
    LeaveCriticalSection(&lock);

    closesocket(sock);
    return 0;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    InitializeCriticalSection(&lock);

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = { AF_INET, htons(8080), INADDR_ANY };
    
    bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_sock, 3);
    printf("Sunucu 8080 portunda hazir...\n");

    while (1) {
        int* new_sock = malloc(sizeof(int));
        *new_sock = accept(server_sock, NULL, NULL);
        _beginthreadex(NULL, 0, handle_patient, new_sock, 0, NULL);
    }

    DeleteCriticalSection(&lock);
    closesocket(server_sock);
    WSACleanup();
    return 0;
}