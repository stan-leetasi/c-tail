// tail.c
// Author: Stanislav Letaši
// Prints out the last n lines from a file or stdin

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LENGTH 4096 // max length of string or line
#define ALLOC_LENGTH 4098

/*
 *Element in circular buffer
 */
typedef struct element_t
{
    char *data;
} element_t;

/*
 *Circular buffer struct
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
 *Creates circular buffer, returns its pointer
 */
buffer_t *cb_create(int n)
{
    if (n <= 0)
    {
        fprintf(stderr, "ERR: line count must be >= 0\n");
        exit(1);
    }

    buffer_t *buffer = malloc(sizeof(buffer_t));

    if (buffer == NULL)
    {
        fprintf(stderr, "ERR: memory allocation failure in cb_create\n");
        exit(1);
    }

    buffer->elements = (element_t *)malloc(n * (sizeof(element_t)));
    if (buffer->elements == NULL)
    {
        fprintf(stderr, "ERR: memory allocation failure in cb_create\n");
        exit(1);
    }

    buffer->size = buffer->start = buffer->end = 0;
    buffer->capacity = n;

    return buffer;
}

void cb_free(buffer_t *cb)
{
    // for each line in cb
    for (int i = 0; i < cb->size; i++)
    {
        free(cb->elements[i % cb->capacity].data);
    }

    free(cb->elements);
    free(cb);
}

/*
 *Inserts line into circular buffer
 */
void cb_put(buffer_t *cb, char *line)
{
    if (cb->size < cb->capacity)
    {
        cb->size = cb->size + 1;
        cb->elements[cb->end].data = malloc(ALLOC_LENGTH * sizeof(char));
        if (cb->elements[cb->end].data == NULL)
        {
            fprintf(stderr, "ERR: memory allocation failure in cb_put\n");
            exit(1);
        }
    }
    else // start overwriting data
        cb->start = (cb->start + 1) % cb->capacity; // increment start pointer
            
    strcpy(cb->elements[cb->end].data, line); // add new line to cb
    cb->end = (cb->end + 1) % cb->capacity;   // increment end pointer

}

/*
 *Returns first element from circular buffer
 */
element_t *cb_get(buffer_t *cb)
{
    if (cb->size == 0)
    {
        return NULL;
    }

    int index = cb->start;

    cb->start = (cb->start + 1) % cb->capacity; // increment start pointer
    return &(cb->elements[index]);
}

void read_stdin(buffer_t *cb)
{
    char c;
    bool first_err = false; // ensures that the error message for a line exceeding the 4096 character limit gets displayed only once
    char *line = malloc(ALLOC_LENGTH * sizeof(char)); // temporary storage for every read line

    if (line == NULL)
    {
        fprintf(stderr, "ERR: memory allocation failure in read_stdin\n");
        exit(1);
    }

    while (fgets(line, MAX_LENGTH, stdin) != NULL) // read lines from stdin
    {

        if (strlen(line) >= MAX_LENGTH-1 && line[MAX_LENGTH-1] != '\n') // line exceeded the 4096 character limit
        {
            if (first_err == false)
            {
                fprintf(stderr, "A line has exceeded the 4096 characters limit, the rest of the line's characters will be ignored\n");
                first_err = true;
            }

            line[MAX_LENGTH-1] = '\n'; // line formatting
            line[MAX_LENGTH] = '\0'; // end the current string

            while ((c = getchar()) != EOF && c != '\n'); // read and discard rest of the line's characters
        }

        cb_put(cb, line);
    }

    free(line);
}

void read_file(char *filename, buffer_t *cb)
{

    if (access(filename, R_OK) != 0) // check if file exists and is readable
    {
        fprintf(stderr, "ERR: file inaccessible\n");
        cb_free(cb);
        exit(1);
    }

    FILE *input = fopen(filename, "r");

    if (input == NULL)
    {
        fprintf(stderr, "ERR: failed to open file\n");
        exit(1);
    }

    if (feof(input))
    {
        fprintf(stderr, "ERR: file is empty\n");
        cb_free(cb);
        exit(1);
    }

    char *line = malloc(ALLOC_LENGTH * sizeof(char));
    if (line == NULL)
    {
        fprintf(stderr, "ERR: memory allocation failure in read_file\n");
        exit(1);
    }

    char c;
    bool first_err = false; // ensures that the error message for a line exceeding the 4096 character limit gets displayed only once

    while (fgets(line, MAX_LENGTH, input) != NULL) // read lines from file
    {
        if (strlen(line) >= MAX_LENGTH-1 && line[MAX_LENGTH-1] != '\n') // line exceeded the 4096 character limit  
        {
            if (first_err == false)
            {
                fprintf(stderr, "A line has exceeded the 4096 characters limit, the rest of the line's characters will be ignored\n");
                first_err = true;
            }

            line[MAX_LENGTH-1] = '\n'; // line formatting
            line[MAX_LENGTH] = '\0'; // end the current string

            while ((c = fgetc(input)) != '\n' && c != EOF); // read and discard rest of the line's characters
        }

        cb_put(cb, line);
    }

    free(line);
    fclose(input);
}

/*
 *Prints last n lines (n=10 by default) from stdin or file
 */
void print_output(buffer_t *cb)
{
    char *line;

    for (int i = 0; i < cb->size; i++)
    {
        line = cb_get(cb)->data;
        if (line == NULL)
        {
            fprintf(stderr, "ERR: cb_get attempt with empty buffer\n");
            cb_free(cb);
            exit(1);
        }
        printf("%s", line);
    }
}

int main(int argc, char *argv[])
{
    buffer_t *cb;

    if (argc == 1) // no input, read from stdin by default
    {
        cb = cb_create(10);
        read_stdin(cb);
        print_output(cb);
        cb_free(cb);
    }

    if (argc == 2) // only filename was input
    {
        cb = cb_create(10);
        read_file(argv[1], cb);
        print_output(cb);
        cb_free(cb);
    }

    if (argc == 3) // only number of lines was input, read from stdin
    {
        if (strcmp(argv[1], "-n") != 0)
        {
            fprintf(stderr, "ERR: wrong input format\n");
            exit(1);
        }
        else
        {
            char *c;
            int num;
            long conv = strtol(argv[2], &c, 10);
            num = conv;

            cb = cb_create(num);
            read_stdin(cb);
            print_output(cb);
            cb_free(cb);
        }
    }

    if (argc == 4) // number of lines and filename was input
    {
        if (strcmp(argv[1], "-n") != 0)
        {
            fprintf(stderr, "ERR: wrong input format\n");
            exit(1);
        }
        else
        {
            char *c;
            int num;
            long conv = strtol(argv[2], &c, 10);
            num = conv;

            cb = cb_create(num);
            read_file(argv[3], cb);
            print_output(cb);
            cb_free(cb);
        }
    }

    if (argc > 4)
    {
        fprintf(stderr, "ERR: wrong input format\n");
        exit(1);
    }

    return 0;
}