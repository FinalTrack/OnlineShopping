#define MAX_SIZE 1024
#define SEM_KEY 1234

struct product
{
    int id;
    char name[256];
    float price;
    int quantity;
};