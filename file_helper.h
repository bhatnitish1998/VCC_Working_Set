#ifndef VCC_WORKING_SET_FILE_HELPER_H
#define VCC_WORKING_SET_FILE_HELPER_H

#include <stdio.h>
#include <fcntl.h>

int read_workload_from_file(const char *filename, int access_size[], int duration[]) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        return 0;
    }

    int rows = 0;
    // Skip the first line (header)
    char line[100];
    fgets(line, sizeof(line), file);

    // Read the rest of the file
    while (fscanf(file, "%d %d", &access_size[rows], &duration[rows]) == 2) {
        rows++;
    }
    fclose(file);

    return rows;
}

#endif //VCC_WORKING_SET_FILE_HELPER_H
