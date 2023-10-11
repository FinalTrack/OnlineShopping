#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "defn.h"

struct product cart[MAX_SIZE];

int main()
{
    printf("Welcome to online shopping! Connecting to server...\n");

    int cd = socket(AF_INET, SOCK_STREAM, 0);
    if(cd < 0)
    {
        perror("Socket error");
        return 1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(8080);

    if(connect(cd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("Connect error");
        return 1;
    }

    printf("Success!\n\n");
    printf("1. Admin\n2. User\n");
    int choice;
    scanf("%i", &choice);
    if(choice == 1)
    {
        while(1)
        {
            printf("\n1. Add\n2. Delete\n3. Update\n4. View Products\n5. Exit\n");
            scanf("%i", &choice);
            if(choice == 1)
            {
                printf("Enter product name (no spaces), price, quantity\n");
                char name[256];
                float price;
                int quantity;
                scanf("%s%f%i", name, &price, &quantity);
                send(cd, &choice, sizeof(choice), 0);
                send(cd, name, sizeof(name), 0);
                send(cd, &price, sizeof(price), 0);
                send(cd, &quantity, sizeof(quantity), 0);

                char msg[256];
                recv(cd, msg, sizeof(msg), 0);
                printf(msg);
            }
            else if(choice == 2)
            {
                printf("Enter product id\n");
                int id;
                scanf("%i", &id);
                send(cd, &choice, sizeof(choice), 0);
                send(cd, &id, sizeof(id), 0);

                char msg[256];
                recv(cd, msg, sizeof(msg), 0);
                printf(msg);
            }
            else if(choice == 3)
            {
                printf("Enter product id, new price, new quantity\n");
                int id, quantity;
                float price;
                scanf("%i%f%i", &id, &price, &quantity);
                send(cd, &choice, sizeof(choice), 0);
                send(cd, &id, sizeof(id), 0);
                send(cd, &price, sizeof(price), 0);
                send(cd, &quantity, sizeof(quantity), 0);

                char msg[256];
                recv(cd, msg, sizeof(msg), 0);
                printf(msg);
            }
            else if(choice == 4)
            {
                send(cd, &choice, sizeof(choice), 0);
                char msg[4096];
                recv(cd, msg, sizeof(msg), 0);
                printf("PRODUCTS: \n");
                printf(msg);
            }
            else
                break;
        }
    }
    else
    {
        int cartCount = 0;
        for(int i = 0; i < MAX_SIZE; i++)
            cart[i].id = -1;
        while(1)
        {
            printf("\n1. View Products\n2. View Cart\n3. Add to Cart\n4. Checkout\n");
            scanf("%i", &choice);
            choice += 3;
            if(choice == 4)
            {
                send(cd, &choice, sizeof(choice), 0);
                char msg[4096];
                recv(cd, msg, sizeof(msg), 0);
                printf("PRODUCTS: \n");
                printf(msg);
            }
            else if(choice == 5)
            {
                printf("CART:\n");
                for(int i = 0; i < cartCount; i++)
                    printf("%i\t%s\t%.2f\t%i\n", cart[i].id, cart[i].name, cart[i].price, cart[i].quantity);
            }
            else if(choice == 6)
            {
                if(cartCount == MAX_SIZE)
                {
                    printf("Cart full!\n");
                    continue;
                }
                printf("Enter product id and quantity\n");
                int id, quantity; scanf("%i%i", &id, &quantity);
                send(cd, &choice, sizeof(choice), 0);
                send(cd, &id, sizeof(id), 0);
                struct product p;
                recv(cd, &p, sizeof(p), 0);
                if(p.id == -1)
                    printf("Given product id is not currently available\n");
                else
                {
                    printf("Added to cart\n");
                    if(p.quantity < quantity)
                        printf("Please note the requested quantity is not currently available");
                    p.quantity = quantity;
                    cart[cartCount] = p;
                    cartCount++;
                }
            }
            else
                break;
        }
        if(!cartCount)
        {
            printf("Cart is empty\n");
            return 0;
        }
        
        printf("\nEdit your cart before proceeding!\n");

        while(1)
        {
            printf("\n1. View Cart\n2. Update\n3. Delete\n4. Proceed to purchase\n");
            scanf("%i", &choice);
            if(choice == 1)
            {
                printf("CART:\n");
                for(int i = 0; i < MAX_SIZE; i++)
                {
                    if(cart[i].id > -1)
                        printf("%i\t%s\t%.2f\t%i\n", cart[i].id, cart[i].name, cart[i].price, cart[i].quantity);
                }
            }
            else if(choice == 2)
            {
                printf("Enter productID and new quantity\n");
                int id, quantity;
                scanf("%i%i", &id, &quantity);
                int i;
                for(i = 0; i < MAX_SIZE; i++)
                {
                    if(cart[i].id == id)
                    {
                        cart[i].quantity = quantity;
                        break;
                    }
                }
                if(i < MAX_SIZE)
                    printf("Successfully updated quantity\n");
                else
                    printf("Unable to find id");
            }
            else if(choice == 3)
            {
                printf("Enter productID\n");
                int id;
                scanf("%i", &id);
                int i;
                for(i = 0; i < MAX_SIZE; i++)
                {
                    if(cart[i].id == id)
                    {
                        cart[i].id = -1;
                        cartCount--;
                        break;
                    }
                }
                if(i < MAX_SIZE)
                    printf("Successfully deleted item from cart\n");
                else
                    printf("Unable to find id");
            }
            else
                break;
        }
        if(!cartCount)
        {
            printf("Cart is empty\n");
            return 0;
        }

        printf("\nEntering payment gateway....\n");
        choice = 7;
        struct product finalCart[cartCount];
        int status[cartCount];

        int j = 0;
        for(int i = 0; i < MAX_SIZE; i++)
        {
            if(cart[i].id >= 0)
            {
                finalCart[j] = cart[i];
                j++;
            }
        }

        send(cd, &choice, sizeof(choice), 0);
        send(cd, &cartCount, sizeof(cartCount), 0);
        send(cd, finalCart, sizeof(finalCart), 0);
        recv(cd, status, sizeof(status), 0);
        recv(cd, finalCart, sizeof(finalCart), 0);

        printf("\nFinal Bill: (Note that items which exceed the quantity have been removed)\n");
        float total = 0;
        for(int i = 0; i < cartCount; i++)
        {
            printf("%i\t%s\t%.2f\t%i\t", finalCart[i].id, finalCart[i].name, finalCart[i].price, finalCart[i].quantity);
            if(status[i])
            {
                printf("SUCCESS\n");
                total += finalCart[i].price * finalCart[i].quantity;
            }
            else
                printf("NOT AVAILABLE\n");
        }
        printf("Total: %.2f\nEnter (y) to confirm\n", total);
        char c[256]; scanf("%s", c);
        send(cd, c, sizeof(c), 0);

        if(c[0] != 'y')
            return 0;
        
        FILE* file;
        file = fopen("receipt.txt", "w");
        for(int i = 0; i < cartCount; i++)
        {
            fprintf(file, "%i\t%s\t%.2f\t%i\t", finalCart[i].id, finalCart[i].name, finalCart[i].price, finalCart[i].quantity);
            if(status[i])
                fprintf(file, "SUCCESS\n");
            else
                fprintf(file, "NOT AVAILABLE\n");
        }
        fprintf(file, "Total: %.2f\n", total);

        printf("\nThank you for shopping. Your receipt has been created\n");

    }

    return 0;
}