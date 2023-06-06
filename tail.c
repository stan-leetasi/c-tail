// tail.c
// Autor: Stanislav Letaši, FIT
// vypisuje posledných n riadkov zo súboru alebo stdin streamu

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LENGTH 4096 // Maximálna dĺžka stringu/riadku
#define ALLOC_LENGTH 4098 // Dĺžka stringu pre allocovanie pamäti

/*
 *Definícia štruktúry jednej položky v circular bufferi
 */
typedef struct element_t
{
    char *data;
} element_t;

/*
 *Definícia štruktúry circular bufferu
 */
typedef struct buffer_t
{
    int capacity;
    int size;
    int start;
    int end;
    element_t *elements;
} buffer_t;

/*
 *Vytvorí circular buffer, vracia ukazateľ na cb
 */
buffer_t *cb_create(int n)
{
    if (n <= 0)
    {
        fprintf(stderr, "Počet riadkov musí byť >= 0\n");
        exit(1);
    }

    buffer_t *buffer = malloc(sizeof(buffer_t));

    if (buffer == NULL)
    {
        fprintf(stderr, "Zlyhala alokacia buffer v cb_create\n");
        exit(1);
    }

    buffer->elements = (element_t *)malloc(n * (sizeof(element_t)));
    if (buffer->elements == NULL)
    {
        fprintf(stderr, "Zlyhala alokacia buffer->elements v cb_create\n");
        exit(1);
    }

    buffer->size = 0;
    buffer->capacity = n;
    buffer->start = 0;
    buffer->end = 0;

    return buffer;
}

/*
 *Uvoľní všetku pamäť používanú circular bufferom
 */
void cb_free(buffer_t *cb)
{

    for (int i = 0; i < cb->size; i++)
    {
        free(cb->elements[i % cb->capacity].data); // Uvoľnenie alokovanej pamäti každého riakdu v bufferi
    }

    free(cb->elements); // Uvoľnenie alokovanej pamäti poľa elements
    free(cb);           // Uvoľnenie alokovanej štruktúry buffer
}

/*
 *Vloží riadok do circular bufferu
 */
void cb_put(buffer_t *cb, char *line)
{
    if (cb->size < cb->capacity) // Kým nie je buffer plný, pripočítava sa k počtu položiek uložených v ňom
    {
        cb->size = cb->size + 1;
        cb->elements[cb->end].data = malloc(ALLOC_LENGTH * sizeof(char)); // Alokovanie pamäte pre riadok, ak ešte nebola
        if (cb->elements[cb->end].data == NULL)
        {
            fprintf(stderr, "Zlyhala alokacia cb->elements[cb->end].data v cb_put\n");
            exit(1);
        }

        strcpy(cb->elements[cb->end].data, line); // Pridanie riadku do bufferu
        cb->end = (cb->end + 1) % cb->capacity;   // Posunutie end na ďalší index
    }
    else
    {
        strcpy(cb->elements[cb->end].data, line);   // Pridanie riadku do bufferu
        cb->end = (cb->end + 1) % cb->capacity;     // Posunutie end na ďalší index
        cb->start = (cb->start + 1) % cb->capacity; // Posunutie start na ďalší index
    }
}

/*
 *Vracia prvú položku v circular buffery
 */
element_t *cb_get(buffer_t *cb)
{
    if (cb->size == 0)
    {
        return NULL;
    }

    int index = cb->start;

    cb->start = (cb->start + 1) % cb->capacity; // Inkrementovanie start indexu
    return &(cb->elements[index]);
}

/*
 *Čítanie vstupu zo stdin
 */
void read_stdin(buffer_t *cb)
{
    char c = getchar(); // Pokus o prečítanie prvého charakteru (v non-blocking móde sa nečaká na zadanie charakteru ak na stdin už nie je vstup)


    // Kód ktorý ukončí program ak nie je spustený so vstupom na stdin
    // Bez neho program čaká na vstup zo stdin
    // Musí byť odkomentovaný aby bol program kompatibilný s UNIX pipe
    /*
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK); // Nastavenie stdin streamu do non-blocking módu
    
    if (c == EOF) // Overenie či stdin nie je prázdny
    {
        fprintf(stderr, "Na stdin nebol zadany vstup\n");
        cb_free(cb);
        exit(1);
    }
    else
    {
        fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0)); // Vrátenie default nastavení stdin streamu
        ungetc(c, stdin);                                              // Vrátenie prečítaného charakteru na stdin
    }
    */

    bool first_err = false; // Pri prvom presiahnutí MAX_LENGTH sa nastaví na true, aby sa správa o prekročení dĺžky vypísala len raz

    char *line = malloc(ALLOC_LENGTH * sizeof(char)); // Do line sa dočasne ukladá každý prečítaný riadok
    if (line == NULL)
    {
        fprintf(stderr, "Zlyhala alokacia line v read_stdin\n");
        exit(1);
    }

    while (fgets(line, MAX_LENGTH, stdin) != NULL) // Čítanie riadkov zo stdin
    {

        if (strlen(line) >= MAX_LENGTH-1 && line[MAX_LENGTH-1] != '\n') // Ak riadok presiahol dĺžku 4096 charakterov
        {
            if (first_err == false)
            {
                fprintf(stderr, "Riadok prekrocil hranicu 4096 charakterov, zvysne charaktery budu ignorovane\n");
                first_err = true;
            }

            line[MAX_LENGTH-1] = '\n'; // Append newline charakteru na koniec riadku kvôli formátu výpisu
            line[MAX_LENGTH] = '\0'; // Append '\0' charakteru na koniec riadku kvôli ukončeniu stringu

            while ((c = getchar()) != EOF && c != '\n'); // Načítanie a zahodenie zvyšných riadkov
        }

        cb_put(cb, line); // Vloženie riadku do bufferu
    }

    free(line); // Uvoľnenie miesta alokovaného pre dočasnú premennú line
}

/*
 *Čítanie vstupu zo súboru
 */
void read_file(char *filename, buffer_t *cb)
{

    if (access(filename, R_OK) != 0) // Overenie či súbor existuje a či sa dá čítať, ak nie, formát vstupu je nesprávny
    {
        fprintf(stderr, "Nespravny format vstupu\n");
        cb_free(cb);
        exit(1);
    }

    FILE *input = fopen(filename, "r");

    if (input == NULL) // Ak zlyhalo otvorenie vstupného suboru
    {
        fprintf(stderr, "Otvorenie vstupneho suboru zlyhalo\n");
        exit(1);
    }

    if (feof(input))
    {
        fprintf(stderr, "Zadany subor je prazdny\n");
        cb_free(cb);
        exit(1);
    }

    char *line = malloc(ALLOC_LENGTH * sizeof(char)); // Do line sa dočasne ukladá každý prečítaný riadok
    if (line == NULL)
    {
        fprintf(stderr, "Zlyhala alokacia line v read_file\n");
        exit(1);
    }
    char c;

    bool first_err = false;

    while (fgets(line, MAX_LENGTH, input) != NULL) // Čítanie riadkov zo suboru
    {
        if (strlen(line) >= MAX_LENGTH-1 && line[MAX_LENGTH-1] != '\n') // Ak riadok presiahol dĺžku 4096 charakterov
        {
            if (first_err == false)
            {
                fprintf(stderr, "Riadok prekrocil hranicu 4096 charakterov, zvysne charaktery budu ignorovane\n");
                first_err = true;
            }

            line[MAX_LENGTH-1] = '\n'; // Append newline charakteru na koniec riadku kvôli formátu výpisu
            line[MAX_LENGTH] = '\0'; // Append '\0' charakteru na koniec riadku kvôli ukončeniu stringu

            while ((c = fgetc(input)) != '\n' && c != EOF); // Načítanie a zahodenie zvyšných charakterov
        }

        cb_put(cb, line); // Vloženie riadku do bufferu
    }

    free(line); // Uvoľnenie miesta alokovaného pre dočasnú premennú line

    fclose(input);
}

/*
 *Vypíše posledných n (default 10) riadkov zo stdin alebo zo súboru
 */
void print_output(buffer_t *cb)
{
    char *line;

    for (int i = 0; i < cb->size; i++) // Výpis všetkých n riadkov
    {
        line = cb_get(cb)->data;
        if (line == NULL)
        {
            fprintf(stderr, "Pokus o cb_get nad prazdnym bufferom\n");
            cb_free(cb);
            exit(1);
        }
        printf("%s", line);
    }
}

int main(int argc, char *argv[])
{
    buffer_t *cb;

    if (argc == 1) // Nebol zadaný vstup, default option je čítanie zo stdin
    {
        cb = cb_create(10);
        read_stdin(cb);
        print_output(cb);
        cb_free(cb);
    }

    if (argc == 2) // Predpokladá sa že bol zadaný len názov súboru
    {
        cb = cb_create(10);
        read_file(argv[1], cb);
        print_output(cb);
        cb_free(cb);
    }

    if (argc == 3) // Predpokladá sa že bol zadaný len počet riadkov a vstup je na stdin
    {
        if (strcmp(argv[1], "-n") != 0)
        {
            fprintf(stderr, "Nespravny format vstupu\n");
            exit(1);
        }
        else
        {
            char *c;
            int num;
            long conv = strtol(argv[2], &c, 10); // Pretypovanie programoveho argumentu na int
            num = conv;

            cb = cb_create(num);
            read_stdin(cb);
            print_output(cb);
            cb_free(cb);
        }
    }

    if (argc == 4) // Predpokladá sa že bol zadaný počet riadkov a názov súboru
    {
        if (strcmp(argv[1], "-n") != 0)
        {
            fprintf(stderr, "Nespravny format vstupu\n");
            exit(1);
        }
        else
        {
            char *c;
            int num;
            long conv = strtol(argv[2], &c, 10); // Pretypovanie programoveho argumentu na int
            num = conv;

            cb = cb_create(num);
            read_file(argv[3], cb);
            print_output(cb);
            cb_free(cb);
        }
    }

    if (argc > 4)
    {
        fprintf(stderr, "Nespravny format vstupu\n");
        exit(1);
    }

    return 0;
}