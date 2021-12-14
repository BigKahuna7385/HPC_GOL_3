#include <mpi.h>
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>

#define calcIndex(width, x, y) ((y) * (width) + (x))

int countNeighbors(int *currentfield, int x, int y, int w);
void readInputConfig(int *currentfield, int width, int height, char inputConfiguration[]);

long TimeSteps = 0;

int isDirectoryExists(const char *path)
{
    struct stat stats;

    stat(path, &stats);

    if (S_ISDIR(stats.st_mode))
        return 1;

    return 0;
}

void writeVTK2(long timestep, int *data, char prefix[1024], int localWidth, int localHeight, int threadNumber, int cartX, int cartY)
{
    int w = localWidth - 2, h = localHeight - 2;
    int originY = cartY * h;
    int originX = cartX * w;
    char filename[2048];
    int x, y;
    if (isDirectoryExists("vti") == 0)
        mkdir("vti", 0777);
    float deltax = 1.0;
    long nxy = w * h * sizeof(float);
    snprintf(filename, sizeof(filename), "vti/%s_%d-%05ld%s", prefix, threadNumber, timestep, ".vti");
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", originX,
            originX + w, originY, originY + h, 0, 0, deltax, deltax, 0.0);
    fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char *)&nxy, sizeof(long), 1, fp);

    for (y = 1; y < localHeight - 1; y++)
    {
        for (x = 1; x < localWidth - 1; x++)
        {
            float value = data[calcIndex(localWidth, x, y)];
            fwrite((unsigned char *)&value, sizeof(float), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

void show(int *currentfield, int w, int h)
{
    printf("\033[H");
    int x, y;
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
            printf(currentfield[calcIndex(w, x, y)] ? "\033[07m  \033[m" : "  ");
        // printf("\033[E");
        printf("\n");
    }
    fflush(stdout);
}

int countNeighbors(int *currentfield, int x, int y, int widthTotal)
{
    int cnt = 0;
    int y1 = y - 1;
    int x1 = x - 1;

    while (y1 <= y + 1)
    {
        if (currentfield[calcIndex(widthTotal, x1, y1)])
            cnt++;

        if (x1 <= x)
            x1++;
        else
        {
            y1++;
            x1 = x - 1;
        }
    }
    return cnt;
}

void evolve(int *field, int *newField, int segmentWidth, int segmentHeight, int my_rank)
{
    int x = 1, y = 1;

    while (y < segmentHeight - 1)
    {
        int neighbors = countNeighbors(field, x, y, segmentWidth);
        int index = calcIndex(segmentWidth, x, y);
        if (field[index])
            neighbors--;

        if (neighbors == 3 || (neighbors == 2 && field[index]))
            newField[index] = 1;
        else
            newField[index] = 0;

        if (x < segmentWidth - 2)
            x++;
        else
        {
            y++;
            x = 1;
        }
    }
}

void filling(int *currentfield, int w, int h, char *inputConfiguration, int myRank)
{
    if (strlen(inputConfiguration) > 0)
    {
        printf("Using starting Data.\n");
        readInputConfig(currentfield, w, h, inputConfiguration);
    }
    else
    {
        int i;
        srand((int)(time(NULL) * myRank));
        for (i = 0; i < h * w; i++)
        {
            if (i % w != 0 && i % w != w - 1 && i % h != 0 && i % h != h - 1)
                currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randoml
        }
    }
}

void readInputConfig(int *currentfield, int width, int height, char *inputConfiguration)
{
    int i = 0;
    int x = 0;
    int y = height - 1;

    char currentCharacter;
    char nextCharacter;
    int number;
    bool isComment = true;

    while (isComment)
    {
        if (inputConfiguration[i] == '#')
        {
            isComment = false;
            while (inputConfiguration[i] != '\n')
                i++;
            isComment = inputConfiguration[++i] == '#';
        }
    }

    int buffIndex = 0;
    char bufferX[10];
    if (inputConfiguration[i] == 'x' || inputConfiguration[i] == 'X')
    {
        while (isdigit(inputConfiguration[i]) == false)
            i++;
        while (isdigit(inputConfiguration[i]))
            bufferX[buffIndex++] = inputConfiguration[i++];
    }
    int xMax = atoi(bufferX);

    while (inputConfiguration[i] != 'y' && inputConfiguration[i] != 'Y')
        i++;

    char bufferY[10];
    buffIndex = 0;
    if (inputConfiguration[i] == 'y' || inputConfiguration[i] == 'Y')
    {
        while (isdigit(inputConfiguration[i]) == false)
            i++;
        while (isdigit(inputConfiguration[i]))
            bufferY[buffIndex++] = inputConfiguration[i++];
        while (inputConfiguration[i] != '\n')
            i++;
    }

    int yMax = atoi(bufferY);

    printf("Max x/y: %d/%d Width/Height: %d/%d\n", xMax, yMax, width, height);

    while (inputConfiguration[i] != '!')
    {
        currentCharacter = inputConfiguration[i];
        if (currentCharacter == '$')
        {
            x = 0;
            y--;
        }
        else if (currentCharacter == 'b')
            x++;
        else if (currentCharacter == 'o')
            currentfield[calcIndex(width, x++, y)] = 1;
        else if (isdigit(currentCharacter))
        {
            char *digitBuffer = calloc(10, sizeof(char));
            int digitBufferCounter = 0;
            while (isdigit(inputConfiguration[i]))
                digitBuffer[digitBufferCounter++] = inputConfiguration[i++];

            number = atoi(digitBuffer);
            nextCharacter = inputConfiguration[i];
            if (nextCharacter == 'b')
                x += number;
            else if (nextCharacter == 'o')
            {
                // printf("Number: %d |", number);
                int upperBound = x + number;
                // printf("UpperBound: %d\n", upperBound);
                for (; x < upperBound; x++)
                {
                    currentfield[calcIndex(width, x, y)] = 1;
                }
            }
            else if (nextCharacter == '$')
            {
                x = 0;
                y -= number;
            }
            else
                printf("Errorchar: %c\n", nextCharacter);
        }
        i++;
    } // printf("The String is: %s | The array size is: %d\n", inputConfiguration, stringLength);
}

void game(int argc, char *argv[], int segmentWidth, int segmentHeight, int threadX, int threadY, char *inputConfiguration)
{
    int *field = calloc(segmentWidth * segmentHeight, sizeof(int));

    MPI_Init(&argc, &argv);
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int dims[2] = {0, 0};
    MPI_Dims_create(size, 2, dims);

    int periods[2] = {true, true};
    int reorder = false;

    MPI_Comm CART_COMM_WORLD;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &CART_COMM_WORLD);

    int my_rank;
    MPI_Comm_rank(CART_COMM_WORLD, &my_rank);
    filling(field, segmentWidth, segmentHeight, inputConfiguration, my_rank);

    int my_coords[2];
    MPI_Cart_coords(CART_COMM_WORLD, my_rank, 2, my_coords);

    int dimensions_full_array[2] = {segmentHeight, segmentWidth};
    int dimensions_subarray_horizontal[2] = {1, segmentWidth};
    int dimensions_subarray_vertical[2] = {segmentHeight, 1};

    int start_coordinates[2] = {0, 0};
    MPI_Datatype ghostlayerLeft;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray_vertical, start_coordinates, MPI_ORDER_C, MPI_INT, &ghostlayerLeft);
    MPI_Type_commit(&ghostlayerLeft);

    start_coordinates[1] = 1;
    MPI_Datatype innerlayerLeft;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray_vertical, start_coordinates, MPI_ORDER_C, MPI_INT, &innerlayerLeft);
    MPI_Type_commit(&innerlayerLeft);

    start_coordinates[1] = segmentWidth - 1;
    MPI_Datatype ghostlayerRight;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray_vertical, start_coordinates, MPI_ORDER_C, MPI_INT, &ghostlayerRight);
    MPI_Type_commit(&ghostlayerRight);

    start_coordinates[1] = segmentWidth - 2;
    MPI_Datatype innerlayerRight;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray_vertical, start_coordinates, MPI_ORDER_C, MPI_INT, &innerlayerRight);
    MPI_Type_commit(&innerlayerRight);

    //------------------------------------------------------------------------

    start_coordinates[1] = 0;
    MPI_Datatype ghostlayerBottom;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray_horizontal, start_coordinates, MPI_ORDER_C, MPI_INT, &ghostlayerBottom);
    MPI_Type_commit(&ghostlayerBottom);

    start_coordinates[0] = 1;
    MPI_Datatype innerlayerBottom;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray_horizontal, start_coordinates, MPI_ORDER_C, MPI_INT, &innerlayerBottom);
    MPI_Type_commit(&innerlayerBottom);

    start_coordinates[0] = segmentHeight - 1;
    MPI_Datatype ghostlayerTop;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray_horizontal, start_coordinates, MPI_ORDER_C, MPI_INT, &ghostlayerTop);
    MPI_Type_commit(&ghostlayerTop);

    start_coordinates[0] = segmentHeight - 2;
    MPI_Datatype innerlayerTop;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray_horizontal, start_coordinates, MPI_ORDER_C, MPI_INT, &innerlayerTop);
    MPI_Type_commit(&innerlayerTop);

    MPI_Datatype innerlayers[4] = {innerlayerTop, innerlayerBottom, innerlayerLeft, innerlayerRight};
    MPI_Datatype ghostlayers[4] = {ghostlayerBottom, ghostlayerTop, ghostlayerRight, ghostlayerLeft};

    // printf("TimeSteps: %ld\n", TimeSteps);
    for (long t = 0; t < TimeSteps; t++)
    {

        MPI_Request requests[8];
        MPI_Status status[8];
        int r0, r1;
        int req = 0;

        for (int dim = 0, side = 0; dim < 2; ++dim)
        {
            MPI_Cart_shift(CART_COMM_WORLD, dim, 1, &r0, &r1);
            MPI_Isend(field, 1, innerlayers[side], r0, 1, CART_COMM_WORLD, &(requests[req++]));
            MPI_Irecv(field, 1, ghostlayers[side], r0, 1, CART_COMM_WORLD, &(requests[req++]));
            ++side;
            MPI_Isend(field, 1, innerlayers[side], r1, 1, CART_COMM_WORLD, &(requests[req++]));
            MPI_Irecv(field, 1, ghostlayers[side], r1, 1, CART_COMM_WORLD, &(requests[req++]));
            ++side;
        }

        MPI_Waitall(req, requests, status);
        int *newField = calloc(segmentWidth * segmentHeight, sizeof(int));
        evolve(field, newField, segmentWidth, segmentHeight, my_rank);
        field = newField;
        writeVTK2(t, field, "gol", segmentWidth, segmentHeight, my_rank, my_coords[1], my_coords[0]);
    }
    MPI_Finalize();
    free(field);
}

int main(int argc, char *argv[])
{
    int segmentWidth = 0;
    int segmentHeight = 0;
    int amountXThreads = 0;
    int amountYThreads = 0;
    char fileName[1024] = "";

    if (argc > 1)
        TimeSteps = atoi(argv[1]);
    if (argc > 2)
        segmentWidth = atoi(argv[2]);
    if (argc > 3)
        segmentHeight = atoi(argv[3]);
    if (segmentWidth <= 0)
        segmentWidth = 256;
    if (segmentHeight <= 0)
        segmentHeight = 256;
    if (TimeSteps <= 0)
        TimeSteps = 100;

    int widthTotal = segmentWidth * amountXThreads;
    int heightTotal = segmentHeight * amountYThreads;

    segmentHeight = segmentHeight + 2;
    segmentWidth = segmentWidth + 2;

    int bufferSize = widthTotal * heightTotal;

    char *readBuffer = calloc(bufferSize, sizeof(char));

    if (argc > 4)
    {
        snprintf(fileName, sizeof(fileName), "%s", argv[6]);

        printf("Filename: %s\n", fileName);
        FILE *fp;
        fp = fopen(fileName, "r");
        if (!fp)
        {
            printf("Could not open File.\n");
            return 1;
        }
        fread(readBuffer, sizeof(char), bufferSize, fp);
        fclose(fp);
    }

    game(argc, argv, segmentWidth, segmentHeight, amountXThreads, amountYThreads, readBuffer);

    return 0;
}
