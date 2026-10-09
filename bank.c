#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h> // For Sleep() and system("cls")

#define DB_FILE "bank_db.txt"
#define XOR_KEY 0x5A

// --- ANSI Color Codes ---
#define C_RESET   "\x1b[0m"
#define C_BOLD    "\x1b[1m"
#define C_RED     "\x1b[31m"
#define C_GREEN   "\x1b[32m"
#define C_YELLOW  "\x1b[33m"
#define C_CYAN    "\x1b[36m"
#define C_BG_BLUE "\x1b[44m"

// --- Data Structures ---
typedef struct {
    int acc_number;
    char name[50];
    double balance;
    unsigned long pin_hash;
} Account;

typedef struct {
    Account *array;
    int count;
    int capacity;
} Database;

// --- Function Prototypes ---
unsigned long djb2_hash(const char *str);
void clear_input_buffer();
int get_valid_int();
double get_valid_double();
void print_header(const char *title);
void pause_and_clear();

void init_db(Database *db);
void free_db(Database *db);
void load_data(Database *db);
void save_data(Database *db);

void create_account(Database *db);
void deposit(Database *db);
void transfer(Database *db);
int find_account(Database *db, int acc_number);
int authenticate(Database *db, int index);

// --- Main Execution ---
int main() {
    Database db;
    init_db(&db);
    load_data(&db);
    srand((unsigned int)time(NULL)); 

    int choice;
    while (1) {
        system("cls"); // Clear screen for clean UI
        
        // Draw Main Menu Panel
        printf(C_CYAN C_BOLD);
        printf("=================================================\n");
        printf("|" C_BG_BLUE "            SECURE C-BANKING SYSTEM            " C_RESET C_CYAN C_BOLD "|\n");
        printf("=================================================\n");
        printf("|                                               |\n");
        printf("|  " C_YELLOW "[1]" C_RESET " Create New Account                       " C_CYAN C_BOLD "|\n");
        printf("|  " C_YELLOW "[2]" C_RESET " Deposit Money                            " C_CYAN C_BOLD "|\n");
        printf("|  " C_YELLOW "[3]" C_RESET " Transfer Money                           " C_CYAN C_BOLD "|\n");
        printf("|  " C_YELLOW "[4]" C_RESET " Exit System                              " C_CYAN C_BOLD "|\n");
        printf("|                                               |\n");
        printf("=================================================\n" C_RESET);
        
        printf(C_YELLOW "\n> Choose an option: " C_RESET);
        choice = get_valid_int();

        switch (choice) {
            case 1: create_account(&db); break;
            case 2: deposit(&db); break;
            case 3: transfer(&db); break;
            case 4: 
                system("cls");
                printf(C_GREEN C_BOLD "\nSaving database and closing safely... Goodbye!\n\n" C_RESET);
                free_db(&db); 
                return 0;
            default: 
                printf(C_RED "\n[X] Invalid choice. Try again.\n" C_RESET);
                pause_and_clear();
        }
    }
    return 0;
}

// --- UI Helpers ---
void print_header(const char *title) {
    system("cls");
    printf(C_CYAN C_BOLD "\n--- %s ---\n\n" C_RESET, title);
}

void pause_and_clear() {
    printf(C_YELLOW "\nPress Enter to return to the main menu..." C_RESET);
    clear_input_buffer(); // Wait for user to press enter
    getchar();
}

// --- Input Sanitization ---
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int get_valid_int() {
    int value;
    while (scanf("%d", &value) != 1) {
        clear_input_buffer();
        printf(C_RED "[X] Invalid input. Please enter a number: " C_RESET);
    }
    return value;
}

double get_valid_double() {
    double value;
    while (scanf("%lf", &value) != 1) {
        clear_input_buffer();
        printf(C_RED "[X] Invalid input. Please enter an amount: " C_RESET);
    }
    return value;
}

// --- Cryptography & Security ---
unsigned long djb2_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; 
    }
    return hash;
}

// --- Dynamic Memory Management ---
void init_db(Database *db) {
    db->capacity = 10;
    db->count = 0;
    db->array = (Account *)malloc(db->capacity * sizeof(Account));
    if (!db->array) exit(1);
}

void free_db(Database *db) {
    free(db->array);
    db->array = NULL;
}

// --- File I/O (Encryption/Decryption) ---
void load_data(Database *db) {
    FILE *file = fopen(DB_FILE, "r");
    if (!file) return; 

    char hex_line[512];
    char buffer[256];

    while (fscanf(file, "%511s", hex_line) == 1) {
        if (db->count >= db->capacity) {
            db->capacity *= 2;
            db->array = (Account *)realloc(db->array, db->capacity * sizeof(Account));
        }

        int len = strlen(hex_line) / 2;
        for (int i = 0; i < len; i++) {
            unsigned int byte;
            sscanf(&hex_line[i * 2], "%02X", &byte);
            buffer[i] = (char)(byte ^ XOR_KEY);
        }
        buffer[len] = '\0';

        sscanf(buffer, "%d %49s %lf %lu", 
               &db->array[db->count].acc_number, 
               db->array[db->count].name, 
               &db->array[db->count].balance, 
               &db->array[db->count].pin_hash);
        db->count++;
    }
    fclose(file);
}

void save_data(Database *db) {
    FILE *file = fopen(DB_FILE, "w");
    if (!file) return;

    char buffer[256];
    for (int i = 0; i < db->count; i++) {
        sprintf(buffer, "%d %s %.2f %lu", 
                db->array[i].acc_number, 
                db->array[i].name, 
                db->array[i].balance, 
                db->array[i].pin_hash);
        
        for (int j = 0; buffer[j] != '\0'; j++) {
            fprintf(file, "%02X", (unsigned char)(buffer[j] ^ XOR_KEY));
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

// --- Banking Logic ---
int find_account(Database *db, int acc_number) {
    for (int i = 0; i < db->count; i++) {
        if (db->array[i].acc_number == acc_number) return i;
    }
    return -1;
}

int authenticate(Database *db, int index) {
    char pin[20];
    printf("Enter 4-digit PIN: ");
    scanf("%19s", pin);
    
    if (djb2_hash(pin) == db->array[index].pin_hash) {
        return 1;
    }
    printf(C_RED "\n[X] Error: Incorrect PIN.\n" C_RESET);
    return 0; 
}

void create_account(Database *db) {
    print_header("CREATE NEW ACCOUNT");

    if (db->count >= db->capacity) {
        db->capacity *= 2;
        db->array = (Account *)realloc(db->array, db->capacity * sizeof(Account));
    }

    Account new_acc;
    new_acc.acc_number = rand() % 90000 + 10000;
    
    printf("Enter account holder name (no spaces): ");
    scanf("%49s", new_acc.name);
    
    char pin[20];
    printf("Create a 4-digit PIN: ");
    scanf("%19s", pin);
    new_acc.pin_hash = djb2_hash(pin); 
    new_acc.balance = 0.0;
    
    db->array[db->count] = new_acc;
    db->count++;
    save_data(db);

    printf(C_GREEN C_BOLD "\n[+] Account created securely!" C_RESET);
    printf(C_CYAN "\n> Your Account Number is: " C_BOLD "%d\n" C_RESET, new_acc.acc_number);
    
    pause_and_clear();
}

void deposit(Database *db) {
    print_header("DEPOSIT FUNDS");

    printf("Enter account number: ");
    int acc_num = get_valid_int();

    int index = find_account(db, acc_num);
    if (index == -1) {
        printf(C_RED "\n[X] Error: Account not found.\n" C_RESET);
        pause_and_clear();
        return;
    }

    printf("Enter amount to deposit: ");
    double amount = get_valid_double();

    if (amount <= 0) {
        printf(C_RED "\n[X] Error: Invalid amount.\n" C_RESET);
        pause_and_clear();
        return;
    }

    db->array[index].balance += amount;
    save_data(db);
    
    printf(C_GREEN C_BOLD "\n[+] Deposit successful!" C_RESET);
    printf(C_CYAN "\n> New balance: $" C_BOLD "%.2f\n" C_RESET, db->array[index].balance);
    
    pause_and_clear();
}

void transfer(Database *db) {
    print_header("WIRE TRANSFER");

    printf("Enter YOUR account number: ");
    int sender_idx = find_account(db, get_valid_int());

    if (sender_idx == -1) {
        printf(C_RED "\n[X] Error: Account not found.\n" C_RESET);
        pause_and_clear();
        return;
    }

    if (!authenticate(db, sender_idx)) {
        pause_and_clear();
        return;
    }

    printf("\nEnter RECEIVER account number: ");
    int receiver_idx = find_account(db, get_valid_int());

    if (receiver_idx == -1 || receiver_idx == sender_idx) {
        printf(C_RED "\n[X] Error: Invalid receiver account.\n" C_RESET);
        pause_and_clear();
        return;
    }

    printf("Enter amount to transfer: ");
    double amount = get_valid_double();

    if (amount <= 0 || db->array[sender_idx].balance < amount) {
        printf(C_RED "\n[X] Error: Insufficient funds. Balance: $%.2f\n" C_RESET, db->array[sender_idx].balance);
        pause_and_clear();
        return;
    }

    db->array[sender_idx].balance -= amount;
    db->array[receiver_idx].balance += amount;
    
    save_data(db);
    
    printf(C_GREEN C_BOLD "\n[+] Transfer of $%.2f successful!\n" C_RESET, amount);
    
    pause_and_clear();
}