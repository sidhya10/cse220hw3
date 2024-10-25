#include "image.h"
#include <string.h>
#include <stdint.h>
#include <ctype.h>

Image *load_image(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL;

    Image *img = malloc(sizeof(Image));
    if (!img) {
        fclose(fp);
        return NULL;
    }

    // Read magic number
    char magic[3];
    if (fscanf(fp, "%2s", magic) != 1 || magic[0] != 'P' || magic[1] != '3') {
        free(img);
        fclose(fp);
        return NULL;
    }

    // Skip whitespace and comments
    int c;
    while ((c = fgetc(fp)) != EOF) {
        if (c == '#') {
            // Skip until end of line
            while ((c = fgetc(fp)) != EOF && c != '\n');
        } else if (!isspace(c)) {
            ungetc(c, fp);
            break;
        }
    }

    // Read dimensions and max value
    int width, height, max_val;
    if (fscanf(fp, "%d %d %d", &width, &height, &max_val) != 3 ||
        width <= 0 || height <= 0 || width > 4096 || height > 4096 ||
        max_val != 255) {
        free(img);
        fclose(fp);
        return NULL;
    }

    img->width = (unsigned short)width;
    img->height = (unsigned short)height;

    // Allocate pixel array
    size_t num_pixels = (size_t)img->width * (size_t)img->height;
    if (num_pixels > SIZE_MAX / sizeof(unsigned char)) {
        free(img);
        fclose(fp);
        return NULL;
    }

    img->pixels = malloc(num_pixels * sizeof(unsigned char));
    if (!img->pixels) {
        free(img);
        fclose(fp);
        return NULL;
    }

    // Read pixel data
    for (size_t i = 0; i < num_pixels; i++) {
        int r, g, b;
        if (fscanf(fp, "%d %d %d", &r, &g, &b) != 3 ||
            r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
            free(img->pixels);
            free(img);
            fclose(fp);
            return NULL;
        }
        img->pixels[i] = (unsigned char)r;  // Store grayscale value
    }

    fclose(fp);
    return img;
}

void delete_image(Image *image) {
    if (image) {
        free(image->pixels);
        free(image);
    }
}

unsigned char get_image_intensity(Image *image, unsigned int row, unsigned int col) {
    if (!image || row >= image->height || col >= image->width) 
        return 0;
    return image->pixels[row * image->width + col];
}

unsigned short get_image_width(Image *image) {
    return image ? image->width : 0;
}

unsigned short get_image_height(Image *image) {
    return image ? image->height : 0;
}

unsigned int hide_message(char *message, char *input_filename, char *output_filename) {
    if (!message || !input_filename || !output_filename) return 0;
    
    Image *img = load_image(input_filename);
    if (!img) return 0;

    unsigned int max_chars = (img->width * img->height) / 8 - 1;
    unsigned int msg_len = strlen(message);
    unsigned int chars_to_hide = (msg_len < max_chars) ? msg_len : max_chars;

    FILE *fp = fopen(output_filename, "w");
    if (!fp) {
        delete_image(img);
        return 0;
    }

    fprintf(fp, "P3\n%d %d\n255\n", img->width, img->height);

    unsigned int bit_idx = 0;
    unsigned int char_idx = 0;
    unsigned char current_char = message[0];

    for (unsigned int i = 0; i < img->height * img->width; i++) {
        unsigned char pixel = img->pixels[i];
        
        if (char_idx <= chars_to_hide) {
            pixel = (pixel & 0xFE) | ((current_char >> (7 - bit_idx)) & 1);
            
            bit_idx++;
            if (bit_idx == 8) {
                bit_idx = 0;
                char_idx++;
                current_char = (char_idx <= chars_to_hide) ? message[char_idx] : '\0';
            }
        }

        fprintf(fp, "%d %d %d", pixel, pixel, pixel);
        if ((i + 1) % img->width == 0)
            fprintf(fp, "\n");
        else
            fprintf(fp, " ");
    }

    fclose(fp);
    delete_image(img);
    return chars_to_hide;
}

char *reveal_message(char *input_filename) {
    if (!input_filename) return NULL;
    
    Image *img = load_image(input_filename);
    if (!img) return NULL;

    // Allocate space for the message
    size_t max_msg_len = (img->width * img->height) / 8;
    char *message = malloc(max_msg_len * sizeof(char));
    if (!message) {
        delete_image(img);
        return NULL;
    }

    unsigned int bit_idx = 0;
    unsigned int char_idx = 0;
    unsigned char current_char = 0;

    // Extract message from image pixels
    for (unsigned int i = 0; i < img->height * img->width && char_idx < max_msg_len - 1; i++) {
        // Get LSB from current pixel
        current_char = (current_char << 1) | (img->pixels[i] & 1);
        bit_idx++;

        if (bit_idx == 8) {
            message[char_idx] = current_char;
            if (current_char == '\0') break;
            
            char_idx++;
            bit_idx = 0;
            current_char = 0;
        }
    }

    // Ensure null termination
    if (char_idx < max_msg_len) {
        message[char_idx] = '\0';
    }

    delete_image(img);
    return message;
}


unsigned int hide_image(char *secret_image_filename, char *input_filename, char *output_filename) {
    Image *secret = load_image(secret_image_filename);
    Image *cover = load_image(input_filename);
    
    if (!secret || !cover) {
        delete_image(secret);
        delete_image(cover);
        return 0;
    }

    unsigned int required_pixels = 16 + (secret->width * secret->height * 8);
    if (required_pixels > cover->width * cover->height || 
        secret->width >= 256 || secret->height >= 256) {
        delete_image(secret);
        delete_image(cover);
        return 0;
    }

    FILE *fp = fopen(output_filename, "w");
    if (!fp) {
        delete_image(secret);
        delete_image(cover);
        return 0;
    }

    fprintf(fp, "P3\n%d %d\n255\n", cover->width, cover->height);

    unsigned int pixel_idx = 0;
    
    // Hide dimensions
    for (int i = 0; i < 16; i++) {
        unsigned char pixel = cover->pixels[pixel_idx];
        unsigned char dim = (i < 8) ? secret->width : secret->height;
        pixel = (pixel & 0xFE) | ((dim >> (7 - (i % 8))) & 1);
        fprintf(fp, "%d %d %d ", pixel, pixel, pixel);
        pixel_idx++;
    }

    // Hide pixel data
    for (unsigned int i = 0; i < secret->height * secret->width; i++) {
        unsigned char secret_pixel = secret->pixels[i];
        
        for (int bit = 7; bit >= 0; bit--) {
            if (pixel_idx >= cover->width * cover->height) break;
            
            unsigned char cover_pixel = cover->pixels[pixel_idx];
            cover_pixel = (cover_pixel & 0xFE) | ((secret_pixel >> bit) & 1);
            fprintf(fp, "%d %d %d ", cover_pixel, cover_pixel, cover_pixel);
            pixel_idx++;
        }
    }

    while (pixel_idx < cover->width * cover->height) {
        unsigned char pixel = cover->pixels[pixel_idx++];
        fprintf(fp, "%d %d %d ", pixel, pixel, pixel);
    }

    fclose(fp);
    delete_image(secret);
    delete_image(cover);
    return 1;
}

void reveal_image(char *input_filename, char *output_filename) {
    Image *img = load_image(input_filename);
    if (!img) return;

    unsigned char width = 0, height = 0;
    unsigned int pixel_idx = 0;

    for (int i = 0; i < 8; i++) {
        width = (width << 1) | (img->pixels[pixel_idx++] & 1);
    }

    for (int i = 0; i < 8; i++) {
        height = (height << 1) | (img->pixels[pixel_idx++] & 1);
    }

    FILE *fp = fopen(output_filename, "w");
    if (!fp) {
        delete_image(img);
        return;
    }

    fprintf(fp, "P3\n%d %d\n255\n", width, height);

    for (unsigned int i = 0; i < width * height; i++) {
        unsigned char pixel = 0;
        
        for (int bit = 0; bit < 8; bit++) {
            if (pixel_idx >= img->width * img->height) break;
            pixel = (pixel << 1) | (img->pixels[pixel_idx++] & 1);
        }
        
        fprintf(fp, "%d %d %d ", pixel, pixel, pixel);
    }

    fclose(fp);
    delete_image(img);
}