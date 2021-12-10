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

int countNeighbors(double *currentfield, int x, int y, int w, int h);
void readInputConfig(double *currentfield, int width, int height, char inputConfiguration[]);

long TimeSteps = 3;

int isDirectoryExists(const char *path)
{
    struct stat stats;

    stat(path, &stats);

    if (S_ISDIR(stats.st_mode))
        return 1;

    return 0;
}

void writeVTK2(long timestep, double *data, char prefix[1024], int localWidth, int localHeight, int threadNumber, int originX, int originY, int totalWidth)
{
    char filename[2048];
    int x, y;
    int w = localWidth - originX, h = localHeight - originY;
    if (isDirectoryExists("vti") == 0)
        mkdir("vti", 0777);
    int offsetX = originX;
    int offsetY = originY;
    float deltax = 1.0;
    long nxy = w * h * sizeof(float);
    snprintf(filename, sizeof(filename), "vti/%s_%d-%05ld%s", prefix, threadNumber, timestep, ".vti");
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offsetX,
            offsetX + w, offsetY, offsetY + h, 0, 0, deltax, deltax, 0.0);
    fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char *)&nxy, sizeof(long), 1, fp);

    for (y = originY; y < localHeight; y++)
    {
        for (x = originX; x < localWidth; x++)
        {
            float value = data[calcIndex(totalWidth, x, y)];
            fwrite((unsigned char *)&value, sizeof(float), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

void show(double *currentfield, int w, int h)
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

int countNeighbors(double *currentfield, int x, int y, int widthTotal, int heightTotal)
{
    int cnt = 0;
    int y1 = y - 1;
    int x1 = x - 1;

    while (y1 <= y + 1)
    {
        if (currentfield[calcIndex(widthTotal, (x1 + widthTotal) % widthTotal, (y1 + heightTotal) % heightTotal)])
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

void evolve(double *currentfield, double *newfield, int widthTotal, int heightTotal, int threadNumber, int nX, int nY, int pX, int pY, long t)
{
    int x, y, xStart;
    int localpX, localpY;

    localpX = threadNumber % pX;
    localpY = threadNumber / pX;

    x = nX * localpX;
    xStart = x;

    int widthLocal = nX * (localpX + 1);
    y = nY * localpY;
    int heightLocal = nY * (localpY + 1);

    int originX = x, originY = y;

    while (y < heightLocal)
    {
        int neighbors = countNeighbors(currentfield, x, y, widthTotal, heightTotal);
        int index = calcIndex(widthTotal, x, y);
        if (currentfield[index])
            neighbors--;

        if (neighbors == 3 || (neighbors == 2 && currentfield[index]))
            newfield[index] = 1;
        else
            newfield[index] = 0;

        if (x < widthLocal - 1)
            x++;
        else
        {
            y++;
            x = xStart;
        }
    }
    writeVTK2(t, currentfield, "gol", widthLocal, heightLocal, threadNumber, originX, originY, widthTotal);
}

void filling(double *currentfield, int w, int h, char *inputConfiguration)
{
    if (strlen(inputConfiguration) > 0)
    {
        printf("Using starting Data.\n");
        readInputConfig(currentfield, w, h, inputConfiguration);
    }
    else
    {
        int i;
        for (i = 0; i < h * w; i++)
        {
            currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
        }
    }
}

void readInputConfig(double *currentfield, int width, int height, char *inputConfiguration)
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

void game(int nX, int nY, int threadX, int threadY, char *inputConfiguration)
{
    int widthTotal = nX * threadX;
    int heightTotal = nY * threadY;

    double *currentfield = calloc(widthTotal * heightTotal, sizeof(double));
    double *newfield = calloc(widthTotal * heightTotal, sizeof(double));

    // printf("Input:\n%s\n", inputConfiguration);

    filling(currentfield, widthTotal, heightTotal, inputConfiguration);

    long t;
    for (t = 0; t < TimeSteps; t++)
    {
        // show(currentfield, widthTotal, heightTotal);

        int threadNumber = 0;
        // printf("Max threads: %d | Used threads: %d | ThreadNr.: %d\n", omp_get_max_threads(), threadY * threadX, threadNumber);
        evolve(currentfield, newfield, widthTotal, heightTotal, threadNumber, nX, nY, threadX, threadY, t);

        // printf("%ld timestep\n", t);

        // writeVTK2(t, currentfield, "gol", widthTotal, heightTotal);

        // usleep(100000);

        /// SWAP
        double *temp = currentfield;
        currentfield = newfield;
        newfield = temp;
    }

    free(currentfield);
    free(newfield);
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
    if (argc > 4)
        amountXThreads = atoi(argv[4]);
    if (argc > 5)
        amountYThreads = atoi(argv[5]);
    if (segmentWidth <= 0)
        segmentWidth = 10;
    if (segmentHeight <= 0)
        segmentHeight = 1;
    if (amountXThreads <= 0)
        amountXThreads = 2;
    if (amountYThreads <= 0)
        amountYThreads = 1;
    if (TimeSteps <= 0)
        TimeSteps = 100;

    int widthTotal = segmentWidth * amountXThreads;
    int heightTotal = segmentHeight * amountYThreads;

    int bufferSize = widthTotal * heightTotal;

    char *readBuffer = calloc(bufferSize, sizeof(char));

    if (argc > 6)
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

    int *field = calloc(segmentWidth * segmentHeight, sizeof(int));

    MPI_Init(&argc, &argv);
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // int dims[2] = {0, 0};
    // MPI_Dims_create(size, 2, dims);
    int dims[1] = {0};
    MPI_Dims_create(size, 1, dims);

    // int periods[2] = {true, true};
    int periods[1] = {true};
    int reorder = true;

    MPI_Comm CART_COMM_WORLD;
    // MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &CART_COMM_WORLD);
    MPI_Cart_create(MPI_COMM_WORLD, 1, dims, periods, reorder, &CART_COMM_WORLD);

    int my_rank;
    MPI_Comm_rank(CART_COMM_WORLD, &my_rank);

    for (int y = 0; y < segmentHeight; y++)
    {
        printf("y: %d  |", y);
        for (int x = 0; x < segmentWidth; x++)
        {
            field[calcIndex(segmentWidth, x, y)] = (x + 1) + (my_rank * 10);
            printf("%d ", (int)field[calcIndex(segmentWidth, x, y)]);
        }
        printf("\n");
    }

    // int my_coords[2];
    // MPI_Cart_coords(CART_COMM_WORLD, my_rank, 2, my_coords);
    int my_coords[1] = {0};
    MPI_Cart_coords(CART_COMM_WORLD, my_rank, 1, my_coords);

    // printf("[MPI process %d] I am located at (%d, %d).\n", my_rank, my_coords[0],my_coords[1]);
    printf("[MPI process %d] I am located at %d.\n", my_rank, my_coords[0]);

    int dimensions_full_array[2] = {1, segmentWidth};
    int dimensions_subarray[2] = {1, 1};

    int start_coordinates[2] = {0, 0};
    MPI_Datatype ghostlayerleft;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray, start_coordinates, MPI_ORDER_C, MPI_INT, &ghostlayerleft);
    MPI_Type_commit(&ghostlayerleft);

    start_coordinates[1] = 1;
    MPI_Datatype innerlayerleft;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray, start_coordinates, MPI_ORDER_C, MPI_INT, &innerlayerleft);
    MPI_Type_commit(&innerlayerleft);

    start_coordinates[1] = segmentWidth - 1;
    MPI_Datatype ghostlayerright;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray, start_coordinates, MPI_ORDER_C, MPI_INT, &ghostlayerright);
    MPI_Type_commit(&ghostlayerright);

    start_coordinates[1] = segmentWidth - 2;
    MPI_Datatype innerlayerright;
    MPI_Type_create_subarray(2, dimensions_full_array, dimensions_subarray, start_coordinates, MPI_ORDER_C, MPI_INT, &innerlayerright);
    MPI_Type_commit(&innerlayerright);

    MPI_Datatype innerlayers[2] = {innerlayerleft, innerlayerright};
    MPI_Datatype ghostlayers[2] = {ghostlayerright, ghostlayerleft};
    MPI_Request requests[4];
    MPI_Status status[4];

    int r0, r1;
    int req = 0;
    for (int dim = 0, side = 0; dim < 1; ++dim)
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

    MPI_Finalize();

    printf("Rank: %d\n", my_rank);
    for (int y = 0; y < segmentHeight; y++)
    {
        printf("y: %d  |", y);
        for (int x = 0; x < segmentWidth; x++)
        {
            printf("%d ", (int)field[calcIndex(segmentWidth, x, y)]);
        }
        printf("\n");
    }

    return 0;
}
