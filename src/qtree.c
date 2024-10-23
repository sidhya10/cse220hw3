#include "qtree.h"
#include <math.h>

static double calculate_rmse(Image *image, unsigned int start_row, 
                           unsigned int start_col, unsigned int height,
                           unsigned int width, double avg_intensity) {
    if (!image) return 0.0;
    
    double sum_squared_diff = 0.0;
    int count = 0;
    
    for (unsigned int i = start_row; i < start_row + height; i++) {
        for (unsigned int j = start_col; j < start_col + width; j++) {
            if (i < image->height && j < image->width) {
                double diff = get_image_intensity(image, i, j) - avg_intensity;
                sum_squared_diff += diff * diff;
                count++;
            }
        }
    }
    
    return (count > 0) ? sqrt(sum_squared_diff / count) : 0.0;
}

static double calculate_average_intensity(Image *image, unsigned int start_row,
                                       unsigned int start_col, unsigned int height,
                                       unsigned int width) {
    if (!image) return 0.0;
    
    double sum = 0.0;
    int count = 0;
    
    for (unsigned int i = start_row; i < start_row + height; i++) {
        for (unsigned int j = start_col; j < start_col + width; j++) {
            if (i < image->height && j < image->width) {
                sum += get_image_intensity(image, i, j);
                count++;
            }
        }
    }
    
    return (count > 0) ? sum / count : 0.0;
}

static QTNode *create_node(Image *image, unsigned int row, unsigned int col,
                          unsigned int height, unsigned int width, double max_rmse) {
    QTNode *node = malloc(sizeof(QTNode));
    if (!node) return NULL;
    
    node->row = row;
    node->col = col;
    node->height = height;
    node->width = width;
    node->child1 = node->child2 = node->child3 = node->child4 = NULL;
    
    double avg = calculate_average_intensity(image, row, col, height, width);
    node->intensity = (unsigned char)avg;
    
    double rmse = calculate_rmse(image, row, col, height, width, avg);
    
    if (rmse > max_rmse && (height > 1 || width > 1)) {
        unsigned int half_height = height / 2;
        unsigned int half_width = width / 2;
        
        if (half_width > 0 && half_height > 0) {
            node->child1 = create_node(image, row, col,
                                     half_height, half_width, max_rmse);
            node->child2 = create_node(image, row, col + half_width,
                                     half_height, width - half_width, max_rmse);
            node->child3 = create_node(image, row + half_height, col,
                                     height - half_height, half_width, max_rmse);
            node->child4 = create_node(image, row + half_height, col + half_width,
                                     height - half_height, width - half_width, max_rmse);
        }
    }
    
    return node;
}

QTNode *create_quadtree(Image *image, double max_rmse) {
    if (!image) return NULL;
    return create_node(image, 0, 0, get_image_height(image), 
                      get_image_width(image), max_rmse);
}

QTNode *get_child1(QTNode *node) { return node ? node->child1 : NULL; }
QTNode *get_child2(QTNode *node) { return node ? node->child2 : NULL; }
QTNode *get_child3(QTNode *node) { return node ? node->child3 : NULL; }
QTNode *get_child4(QTNode *node) { return node ? node->child4 : NULL; }

unsigned char get_node_intensity(QTNode *node) {
    return node ? node->intensity : 0;
}

void delete_quadtree(QTNode *root) {
    if (!root) return;
    
    delete_quadtree(root->child1);
    delete_quadtree(root->child2);
    delete_quadtree(root->child3);
    delete_quadtree(root->child4);
    
    free(root);
}

static void fill_pixels_from_qtree(QTNode *node, unsigned char *pixels, unsigned int image_width) {
    if (!node || !pixels) return;
    
    // If leaf node, fill region with node's intensity
    if (!node->child1 && !node->child2 && !node->child3 && !node->child4) {
        for (unsigned int i = node->row; i < node->row + node->height; i++) {
            for (unsigned int j = node->col; j < node->col + node->width; j++) {
                pixels[i * image_width + j] = node->intensity;
            }
        }
        return;
    }
    
    // Recursively fill children's regions
    if (node->child1) fill_pixels_from_qtree(node->child1, pixels, image_width);
    if (node->child2) fill_pixels_from_qtree(node->child2, pixels, image_width);
    if (node->child3) fill_pixels_from_qtree(node->child3, pixels, image_width);
    if (node->child4) fill_pixels_from_qtree(node->child4, pixels, image_width);
}

void save_qtree_as_ppm(QTNode *root, char *filename) {
    if (!root || !filename) return;
    
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    
    // Write PPM header
    fprintf(fp, "P3\n%u %u\n255\n", root->width, root->height);
    
    // Create temporary buffer for pixel data
    unsigned char *pixels = calloc(root->width * root->height, sizeof(unsigned char));
    if (!pixels) {
        fclose(fp);
        return;
    }
    
    // Fill buffer with intensities
    fill_pixels_from_qtree(root, pixels, root->width);
    
    // Write pixel data
    for (unsigned int i = 0; i < root->height * root->width; i++) {
        unsigned char intensity = pixels[i];
        fprintf(fp, "%u %u %u ", intensity, intensity, intensity);
        if ((i + 1) % root->width == 0) fprintf(fp, "\n");
    }
    
    free(pixels);
    fclose(fp);
}

static void save_preorder_qt_recursive(QTNode *node, FILE *fp) {
    if (!node || !fp) return;
    
    // Determine if node is leaf or internal
    char type = (node->child1 || node->child2 || node->child3 || node->child4) ? 'N' : 'L';
    
    // Write node data
    fprintf(fp, "%c %u %u %u %u %u\n", type, node->intensity, node->row, 
            node->height, node->col, node->width);
    
    // Recursively write children if this is an internal node
    if (type == 'N') {
        if (node->child1) save_preorder_qt_recursive(node->child1, fp);
        if (node->child2) save_preorder_qt_recursive(node->child2, fp);
        if (node->child3) save_preorder_qt_recursive(node->child3, fp);
        if (node->child4) save_preorder_qt_recursive(node->child4, fp);
    }
}

void save_preorder_qt(QTNode *root, char *filename) {
    if (!root || !filename) return;
    
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    
    save_preorder_qt_recursive(root, fp);
    
    fclose(fp);
}

static QTNode *load_preorder_qt_recursive(FILE *fp) {
    if (!fp) return NULL;
    
    char type;
    unsigned int intensity, row, height, col, width;
    
    if (fscanf(fp, " %c %u %u %u %u %u\n", &type, &intensity, &row, 
               &height, &col, &width) != 6)
        return NULL;
    
    QTNode *node = malloc(sizeof(QTNode));
    if (!node) return NULL;
    
    node->intensity = intensity;
    node->row = row;
    node->height = height;
    node->col = col;
    node->width = width;
    node->child1 = node->child2 = node->child3 = node->child4 = NULL;
    
    if (type == 'N') {
        node->child1 = load_preorder_qt_recursive(fp);
        node->child2 = load_preorder_qt_recursive(fp);
        node->child3 = load_preorder_qt_recursive(fp);
        node->child4 = load_preorder_qt_recursive(fp);
    }
    
    return node;
}

QTNode *load_preorder_qt(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL;
    
    QTNode *root = load_preorder_qt_recursive(fp);
    
    fclose(fp);
    return root;
}