#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

void clear_input() {
    int c; while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Sunucuya baglanilamadi!\n");
        return 1;
    }

    char buffer[1024];
    int choice;

    while (1) {
        system("cls"); // Ekrani temizle
        printf("==========================================\n");
        printf("      DOKTOR RANDEVU SISTEMI             \n");
        printf("==========================================\n");
        printf(" 1. Randevulari Goruntule\n");
        printf(" 2. Randevu Al\n");
        printf(" 3. Randevu Iptal Et\n");
        printf(" 4. Cikis\n");
        printf("------------------------------------------\n");
        printf("Seciminiz > ");

        if (scanf("%d", &choice) != 1) { clear_input(); continue; }
        sprintf(buffer, "%d", choice);
        send(sock, buffer, (int)strlen(buffer), 0);

        if (choice == 4) break;

        system("cls"); // Sonuc ekranini temizle
        if (choice == 2) {
            // Randevu Alma Akisi
            recv(sock, buffer, 1024, 0); printf("%s", buffer); // Dr No prompt
            scanf("%s", buffer); send(sock, buffer, (int)strlen(buffer), 0);
            
            recv(sock, buffer, 1024, 0); printf("%s", buffer); // App No prompt
            scanf("%s", buffer); send(sock, buffer, (int)strlen(buffer), 0);

            int b = recv(sock, buffer, 1024, 0); buffer[b] = '\0';
            printf("\nSONUC: %s\n", buffer);
            printf("\n2 saniye icinde menuye donuluyor...");
            Sleep(2000);
        } else {
            // Goruntuleme veya Iptal
            int b = recv(sock, buffer, 1024, 0); buffer[b] = '\0';
            printf("--- SUNUCU YANITI ---\n\n%s\n", buffer);
            printf("---------------------\n");
            printf("\nMenuye donmek icin [ENTER] tusuna basin...");
            clear_input(); getchar();
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
