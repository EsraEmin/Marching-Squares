// Author: APD team, except where source was noted

#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#define CONTOUR_CONFIG_COUNT    16
#define FILENAME_MAX_SIZE       50
#define STEP                    8
#define SIGMA                   200
#define RESCALE_X               2048
#define RESCALE_Y               2048

#define CLAMP(v, min, max) if(v < min) { v = min; } else if(v > max) { v = max; }

pthread_barrier_t barrier1;


typedef struct {
    int id_thread;
    int nr_thread;
    ppm_image *image;
    ppm_image *new_image;
    ppm_image **contour_map;
    char **grid;
    int step_x;
    int step_y;
    unsigned char sigma;
} ThreadInfo;

ppm_image **init_contour_map() {
    ppm_image **map = (ppm_image **)malloc(CONTOUR_CONFIG_COUNT * sizeof(ppm_image *));
    if (!map) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
        char filename[FILENAME_MAX_SIZE];
        sprintf(filename, "../checker/contours/%d.ppm", i);
        map[i] = read_ppm(filename);
    }

    return map;
}

void tema (void *INFO){
    
    ThreadInfo info = *(ThreadInfo *)INFO;
    uint8_t sample[3];
    if (info.image->x > RESCALE_X || info.image->y > RESCALE_Y) {

    info.new_image->x = RESCALE_X;
    info.new_image->y = RESCALE_Y;
    int startt = info.id_thread * (double)info.new_image->x / info.nr_thread;
    int endd = fmin((info.id_thread + 1) * (double)info.new_image->x / info.nr_thread, info.new_image->x);

    for (int i = startt; i < endd; i++) {

        for (int j = 0; j < info.new_image->y; j++) {

            float u = (float)i / (float)(info.new_image->x - 1);
            float v = (float)j / (float)(info.new_image->y - 1);
            sample_bicubic(info.image, u, v, sample);

            info.new_image->data[i * info.new_image->y + j].red = sample[0];
            info.new_image->data[i * info.new_image->y + j].green = sample[1];
            info.new_image->data[i * info.new_image->y + j].blue = sample[2];
        }
    }

    }
    pthread_barrier_wait(&barrier1);

    if(info.image->x > 2048 || info.image->y >2048){
        info.image = info.new_image;
    }

    int p = info.image->x / info.step_x;
    int q = info.image->y / info.step_y;
    int start = info.id_thread * (double)p / info.nr_thread;
    int end = fmin((info.id_thread + 1) * (double)p / info.nr_thread, p);

    for (int i = start; i < end; i++) {

        for (int j = 0; j < q; j++) {

            ppm_pixel curr_pixel = info.image->data[i * info.step_x * info.image->y + j * info.step_y];

            unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

            if (curr_color > info.sigma) {
                info.grid[i][j] = 0;
            } else {
                info.grid[i][j] = 1;
            }
        }
    }

   if(info.id_thread == info.nr_thread -1){

    for (int i = 0; i < p; i++) {

        ppm_pixel curr_pixel = info.image->data[i * info.step_x * info.image->y + info.image->x - 1];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > info.sigma) {
                info.grid[i][q] = 0;
            } else {
                info.grid[i][q] = 1;
            }
    }
    for (int j = 0; j < q; j++) {

        ppm_pixel curr_pixel = info.image->data[(info.image->x - 1) * info.image->y + j * info.step_y];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > info.sigma) {
                info.grid[p][j] = 0;
            } else {
                info.grid[p][j] = 1;
            }
    }
   }
    pthread_barrier_wait(&barrier1);

    for (int i = start; i < end; i++) {

        for (int j = 0; j < q; j++) {

            unsigned char k = 8 * info.grid[i][j] + 4 * info.grid[i][j + 1] + 2 * info.grid[i + 1][j + 1] + 1 * info.grid[i + 1][j];
            update_image(info.image, info.contour_map[k], i * info.step_x, j * info.step_y);
        }
    }
    pthread_exit(NULL);

}

void update_image(ppm_image *image, ppm_image *contour, int x, int y) {

    for (int i = 0; i < contour->x; i++) {
        for (int j = 0; j < contour->y; j++) {
            int contour_pixel_index = contour->x * i + j;
            int image_pixel_index = (x + i) * image->y + y + j;

            image->data[image_pixel_index].red = contour->data[contour_pixel_index].red;
            image->data[image_pixel_index].green = contour->data[contour_pixel_index].green;
            image->data[image_pixel_index].blue = contour->data[contour_pixel_index].blue;
        }
    }
}


void free_resources(ppm_image *image, ppm_image **contour_map, unsigned char **grid, int step_x) {

    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
        free(contour_map[i]->data);
        free(contour_map[i]);
    }
    free(contour_map);

    for (int i = 0; i <= image->x / step_x; i++) {
        free(grid[i]);
    }
    free(grid);

    free(image->data);
    free(image);
}


int main(int argc, char *argv[]) {

    if (argc < 4) {
        fprintf(stderr, "Usage: ./tema1 <in_file> <out_file> <P>\n");
        return 1;
    }

    ppm_image *image = read_ppm(argv[1]);
    int step_x = STEP;
    int step_y = STEP;

    ppm_image **contour_map = init_contour_map();

    int NUM_THREADS =  atoi(argv[3]);
    ThreadInfo thread_info[NUM_THREADS];
    pthread_t threads[NUM_THREADS];
    pthread_barrier_init(&barrier1, NULL, NUM_THREADS);

    ppm_image *scaled_image = (ppm_image *)malloc(sizeof(ppm_image));
    if (!scaled_image) {
        fprintf(stderr, "Unable to allocate memory\n");
        return 1;
    }

    scaled_image->data = (ppm_pixel *)malloc(RESCALE_X * RESCALE_Y * sizeof(ppm_pixel));
    if (!scaled_image->data) {
        fprintf(stderr, "Unable to allocate memory\n");
        return 1;
    }

    scaled_image->x = RESCALE_X;
    scaled_image->y = RESCALE_Y;

    unsigned char **grid = (unsigned char **)malloc((image->x / STEP + 1) * sizeof(unsigned char *));
    if (!grid) {
        fprintf(stderr, "Unable to allocate memory\n");
        return 1;
    }

    for (int i = 0; i <= image->x / STEP; i++) {
        grid[i] = (unsigned char *)malloc((image->y / STEP + 1) * sizeof(unsigned char));
        if (!grid[i]) {
            fprintf(stderr, "Unable to allocate memory\n");
            return 1;
        }
    }


    for (int i = 0; i < NUM_THREADS; i++){
        thread_info[i].id_thread = i;
        thread_info[i].nr_thread = NUM_THREADS;
        thread_info[i].image = image;
        thread_info[i].new_image = scaled_image;
        thread_info[i].contour_map = contour_map;
        thread_info[i].grid = grid;
        thread_info[i].step_x = STEP;
        thread_info[i].step_y = STEP;
        thread_info[i].sigma = SIGMA;
        pthread_create(&threads[i], NULL,(void *(*)(void *))tema, (void *)&thread_info[i]);
    }

    for(int i = 0; i< NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }

    if(image->x > 2048 || image->y >2048){

    write_ppm(scaled_image, argv[2]);

    } else
    {
        write_ppm(image, argv[2]);
    }


    free_resources(image, contour_map, grid, step_x);

    pthread_barrierattr_destroy(&barrier1);
    return 0;
}
